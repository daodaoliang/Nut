#include <QtTest>
#include <QJsonDocument>
#include <QSqlError>

#include "consts.h"

#include "maintest.h"
#include "query.h"
#include "tableset.h"
#include "tablemodel.h"
#include "databasemodel.h"

#include "post.h"
#include "comment.h"

MainTest::MainTest(QObject *parent) : QObject(parent)
{

}

void MainTest::initTestCase()
{
    qDebug() << "User type id:" << qRegisterMetaType<Post*>();
    qDebug() << "Comment type id:" << qRegisterMetaType<Comment*>();
    qDebug() << "DB type id:" << qRegisterMetaType<WeblogDatabase*>();

    db.setDriver(DRIVER);
    db.setHostName(HOST);
    db.setDatabaseName("nut_tst_basic");
    db.setUserName(USERNAME);
    db.setPassword(PASSWORD);

    bool ok = db.open();

    db.comments()->query()->remove();
    db.posts()->query()->remove();

    QTEST_ASSERT(ok);
}

void MainTest::dataScheema()
{
    auto json = db.model().toJson();
    auto model = DatabaseModel::fromJson(json);

    //    qDebug() << model.toJson();
    //    qDebug() << db.model().toJson();
    QTEST_ASSERT(model == db.model());
}

void MainTest::createPost()
{
    Post *newPost = new Post;
    newPost->setTitle("post title");
    newPost->setSaveDate(QDateTime::currentDateTime());

    db.posts()->append(newPost);

    for(int i = 0 ; i < 3; i++){
        Comment *comment = new Comment;
        comment->setMessage("comment #" + QString::number(i));
        comment->setSaveDate(QDateTime::currentDateTime());
        newPost->comments()->append(comment);
    }
    db.saveChanges();

    postId = newPost->id();

    QTEST_ASSERT(newPost->id() != 0);
    qDebug() << "New post inserted with id:" << newPost->id();
}

void MainTest::createPost2()
{
    Post *newPost = new Post;
    newPost->setTitle("post title");
    newPost->setSaveDate(QDateTime::currentDateTime());

    db.posts()->append(newPost);
    db.saveChanges();

    for(int i = 0 ; i < 3; i++){
        Comment *comment = new Comment;
        comment->setMessage("comment #" + QString::number(i));
        comment->setSaveDate(QDateTime::currentDateTime());
        comment->setPostId(newPost->id());
        db.comments()->append(comment);
    }
    db.saveChanges();

    QTEST_ASSERT(newPost->id() != 0);
    qDebug() << "New post2 inserted with id:" << newPost->id();
}

void MainTest::selectPosts()
{
    auto q = db.posts()->query();
    q->join(Post::commentsTable());
    q->orderBy(!Post::saveDateField() & Post::bodyField());
    q->setWhere(Post::idField() == postId);

    auto posts = q->toList();

    post = posts.at(0);
    post->setBody("");

    QTEST_ASSERT(posts.length() == 1);
    QTEST_ASSERT(posts.at(0)->comments()->length() == 3);
    QTEST_ASSERT(posts.at(0)->title() == "post title");

    QTEST_ASSERT(posts.at(0)->comments()->at(0)->message() == "comment #0");
    QTEST_ASSERT(posts.at(0)->comments()->at(1)->message() == "comment #1");
    QTEST_ASSERT(posts.at(0)->comments()->at(2)->message() == "comment #2");
    db.cleanUp();
}

void MainTest::selectPostsWithoutTitle()
{
    auto q = db.posts()->query();
    q->setWhere(Post::titleField().isNull());
    auto count = q->count();
    qDebug() << q->sqlCommand();
    QTEST_ASSERT(count == 0);
}

void MainTest::selectPostIds()
{
    auto ids = db.posts()->query()->select(Post::idField());

    QTEST_ASSERT(ids.count() == 2);
}

void MainTest::testDate()
{
    QDateTime d = QDateTime::currentDateTime();
    QTime t = QTime(d.time().hour(), d.time().minute(), d.time().second());
    d.setTime(t);

    Post *newPost = new Post;
    newPost->setTitle("post title");
    newPost->setSaveDate(d);

    db.posts()->append(newPost);

    db.saveChanges();

    auto q = db.posts()->query()
            ->setWhere(Post::idField() == newPost->id())
            ->first();

    QTEST_ASSERT(q->saveDate() == d);
}


void MainTest::selectWithInvalidRelation()
{
    auto q = db.posts()->query();
    q->join("Invalid_Class_Name");
    q->toList();
}

void MainTest::select10NewstPosts()
{
    auto q = db.posts()->query();
    q->orderBy(!Post::saveDateField());
    q->toList(10);
}

void MainTest::modifyPost()
{
    auto q = db.posts()->query();
    q->setWhere(Post::idField() == postId);

    Post *post = q->first();

    QTEST_ASSERT(post != 0);

    post->setTitle("new name");
    db.saveChanges();

    q = db.posts()->query()
            ->setWhere(Post::idField() == postId);

    post = q->first();
    QTEST_ASSERT(post->title() == "new name");
}

void MainTest::emptyDatabase()
{
    auto commentsCount = db.comments()->query()->remove();
    auto postsCount = db.posts()->query()->remove();

    QTEST_ASSERT(postsCount == 3);
    QTEST_ASSERT(commentsCount == 6);
}

QTEST_MAIN(MainTest)
