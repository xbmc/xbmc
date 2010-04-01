/* Copyright (c) 2006-2010, Linden Research, Inc.
 * 
 * LLQtWebKit Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in GPL-license.txt in this distribution, or online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/technology-programs/license-virtual-world/viewerlicensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#include <QtTest/QtTest>
#include <trie_p.h>

class tst_Trie : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void trie_data();
    void trie();

    void insert_data();
    void insert();
    void clear();
    void find_data();
    void find();
    void remove_data();
    void remove();
    void all();
};

// Subclass that exposes the protected functions.
class SubTrie : public Trie<int>
{
public:

};

// This will be called before the first test function is executed.
// It is only called once.
void tst_Trie::initTestCase()
{
}

// This will be called after the last test function is executed.
// It is only called once.
void tst_Trie::cleanupTestCase()
{
}

// This will be called before each test function is executed.
void tst_Trie::init()
{
}

// This will be called after every test function.
void tst_Trie::cleanup()
{
}

void tst_Trie::trie_data()
{
}

void tst_Trie::trie()
{
    SubTrie t;
    t.clear();
    QCOMPARE(t.find(QStringList()), QList<int>());
    QCOMPARE(t.remove(QStringList(), -1), false);
    QCOMPARE(t.all(), QList<int>());
    t.insert(QStringList(), -1);
}

void tst_Trie::insert_data()
{
#if 0
    QTest::addColumn<QStringList>("key");
    QTest::addColumn<T>("value");
    QTest::newRow("null") << QStringList() << T();
#endif
}

// public void insert(QStringList const& key, T value)
void tst_Trie::insert()
{
#if 0
    QFETCH(QStringList, key);
    QFETCH(T, value);

    SubTrie<T> t>;

    t>.insert(key, value);
#endif
    QSKIP("Test is not implemented.", SkipAll);
}

// public void clear()
void tst_Trie::clear()
{
    SubTrie t;
    t.insert(QStringList(), 0);
    t.clear();
    QCOMPARE(t.find(QStringList()), QList<int>());
    QCOMPARE(t.all(), QList<int>());
}

Q_DECLARE_METATYPE(QStringList)
typedef QList<int> IntList;
Q_DECLARE_METATYPE(IntList)
void tst_Trie::find_data()
{
    QTest::addColumn<QStringList>("keys");
    QTest::addColumn<IntList>("values");
    QTest::addColumn<QStringList>("find");
    QTest::addColumn<IntList>("found");

    QTest::newRow("null") << QStringList() << IntList() << QStringList() << IntList();

    QStringList wiki = (QStringList() << "t,e,a" << "i" << "t,e,n" << "i,n" << "i,n,n" << "t,o");
    IntList wikiNum = (IntList() << 3 << 11 << 12 << 5 << 9 << 7);

    QTest::newRow("wikipedia-0")
        << wiki
        << wikiNum
        << (QStringList() << "t")
        << (IntList());

    QTest::newRow("wikipedia-1")
        << wiki
        << wikiNum
        << (QStringList() << "t" << "o")
        << (IntList() << 7);

    QTest::newRow("wikipedia-2")
        << (wiki << "t,o")
        << (wikiNum << 4)
        << (QStringList() << "t" << "o")
        << (IntList() << 7 << 4);

    QTest::newRow("wikipedia-3")
        << wiki
        << wikiNum
        << (QStringList() << "i" << "n" << "n")
        << (IntList() << 9);

}

// public QList<T> const find(QStringList const& key)
void tst_Trie::find()
{
    QFETCH(QStringList, keys);
    QFETCH(IntList, values);
    QFETCH(QStringList, find);
    QFETCH(IntList, found);

    SubTrie t;
    for (int i = 0; i < keys.count(); ++i)
        t.insert(keys[i].split(","), values[i]);
    QCOMPARE(t.all(), values);
    QCOMPARE(t.find(find), found);
}

void tst_Trie::remove_data()
{
    QTest::addColumn<QStringList>("keys");
    QTest::addColumn<IntList>("values");
    QTest::addColumn<QStringList>("removeKey");
    QTest::addColumn<int>("removeValue");
    QTest::addColumn<bool>("removed");

    QTest::newRow("null") << QStringList() << IntList() << QStringList() << -1 << false;

    QStringList wiki = (QStringList() << "t,e,a" << "i" << "t,e,n" << "i,n" << "i,n,n" << "t,o");
    IntList wikiNum = (IntList() << 3 << 11 << 12 << 5 << 9 << 7);

    QTest::newRow("valid key-0")
        << wiki
        << wikiNum
        << (QStringList() << "t")
        << -1
        << false;

    QTest::newRow("valid key-1")
        << wiki
        << wikiNum
        << (QStringList() << "t" << "o")
        << -1
        << false;

    QTest::newRow("valid key-2")
        << wiki
        << wikiNum
        << (QStringList() << "t" << "o" << "w")
        << 2
        << false;

    QTest::newRow("valid key-3")
        << wiki
        << wikiNum
        << (QStringList() << "t" << "o")
        << 7
        << true;

    QTest::newRow("valid key-4")
        << wiki
        << wikiNum
        << (QStringList() << "i" << "n")
        << 3
        << false;

    QTest::newRow("valid key-5")
        << wiki
        << wikiNum
        << (QStringList() << "i" << "n")
        << 5
        << true;

}

// public bool remove(QStringList const& key, T value)
void tst_Trie::remove()
{
    QFETCH(QStringList, keys);
    QFETCH(IntList, values);
    QFETCH(QStringList, removeKey);
    QFETCH(int, removeValue);
    QFETCH(bool, removed);

    SubTrie t;
    for (int i = 0; i < keys.count(); ++i)
        t.insert(keys[i].split(","), values[i]);
    QCOMPARE(t.all(), values);
    QCOMPARE(t.remove(removeKey, removeValue), removed);
    if (removed)
        values.removeOne(removeValue);
    QCOMPARE(t.all(), values);
}

void tst_Trie::all()
{
    SubTrie t;
    // hmm everyone else tests this it seems
    QSKIP("Test is not implemented.", SkipAll);
}

QTEST_MAIN(tst_Trie)
#include "tst_trie.moc"

