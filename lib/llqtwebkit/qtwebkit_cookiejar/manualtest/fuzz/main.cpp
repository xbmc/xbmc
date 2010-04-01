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

#include <QtCore/QtCore>
#include <trie_p.h>

QStringList generateKey() {
    QStringList key;
    int size = qrand() % 20 + 3;
    while (key.count() < size)
        key += QString(QChar::fromAscii(qrand() % 26 + 64));
    return key;
}

void basicCheck() {
    QStringList list;
    list << QLatin1String("to") << QLatin1String("tea") << QLatin1String("ten") << QLatin1String("i") << QLatin1String("in") << QLatin1String("inn");
    Trie<int> trie;
    for (int i = 0; i < list.count(); ++i) {
        QString key = list[i];
        QStringList keyList;
        for (int j = 0; j < key.count(); ++j)
            keyList.append(QString(key[j]));
        trie.insert(keyList, i);
    }
    QByteArray data;
    {
        QDataStream stream(&data, QIODevice::ReadWrite);
        stream << trie;
    }
    Trie<int> trie2;
    {
        QDataStream stream(&data, QIODevice::ReadOnly);
        stream >> trie2;
    }
    for (int i = 0; i < list.count(); ++i) {
        QString key = list[i];
        QStringList keyList;
        for (int j = 0; j < key.count(); ++j)
            keyList.append(QString(key[j]));
        QList<int> x = trie2.find(keyList);
        qDebug() << x.count() << i << x[0] << i;
        qDebug() << trie2.remove(keyList, i);
        qDebug() << trie2.find(keyList).count();
    }
}

int main(int argc, char **argv) {
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    basicCheck();

    QHash<QString, int> hash;
    Trie<int> t;
    while (hash.count() < 500) {
        qDebug() << hash.count();
        QStringList key = generateKey();
        int value = qrand() % 50000;
        hash[key.join(QLatin1String(","))] = value;
        t.insert(key, value);

        QHashIterator<QString, int> i(hash);
        while (i.hasNext()) {
            i.next();
            if (t.find(i.key().split(QLatin1Char(','))).count() == 0)
                qDebug() << i.key();
            Q_ASSERT(t.find(i.key().split(QLatin1Char(','))).count() > 0);
            if (qrand() % 500 == 0) {
                t.remove(i.key().split(QLatin1Char(',')), i.value());
                hash.remove(i.key());
            }
            //cout << i.key() << ": " << i.value() << endl;
        }
    }
    return 0;
}

