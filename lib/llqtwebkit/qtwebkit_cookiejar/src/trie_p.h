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

#ifndef TRIE_H
#define TRIE_H

//#define TRIE_DEBUG

#include <qstringlist.h>

#if defined(TRIE_DEBUG)
#include <qdebug.h>
#endif

/*
    A Trie tree (prefix tree) where the lookup takes m in the worst case.

    The key is stored in *reverse* order

    Example:
    Keys: x,a y,a

    Trie:
    a
    | \
    x  y
*/

template<class T>
class Trie {
public:
    Trie();
    ~Trie();

    void clear();
    void insert(const QStringList &key, const T &value);
    bool remove(const QStringList &key, const T &value);
    QList<T> find(const QStringList &key) const;
    QList<T> all() const;

    inline bool contains(const QStringList &key) const;
    inline bool isEmpty() const { return children.isEmpty() && values.isEmpty(); }

private:
    const Trie<T>* walkTo(const QStringList &key) const;
    Trie<T>* walkTo(const QStringList &key, bool create = false);

    template<class T1> friend QDataStream &operator<<(QDataStream &, const Trie<T1>&);
    template<class T1> friend QDataStream &operator>>(QDataStream &, Trie<T1>&);

    QList<T> values;
    QStringList childrenKeys;
    QList<Trie<T> > children;
};

template<class T>
Trie<T>::Trie() {
}

template<class T>
Trie<T>::~Trie() {
}

template<class T>
void Trie<T>::clear() {
#if defined(TRIE_DEBUG)
    qDebug() << "Trie::" << __FUNCTION__;
#endif
    values.clear();
    childrenKeys.clear();
    children.clear();
}

template<class T>
bool Trie<T>::contains(const QStringList &key) const {
    return walkTo(key);
}

template<class T>
void Trie<T>::insert(const QStringList &key, const T &value) {
#if defined(TRIE_DEBUG)
    qDebug() << "Trie::" << __FUNCTION__ << key << value;
#endif
    Trie<T> *node = walkTo(key, true);
    if (node)
        node->values.append(value);
}

template<class T>
bool Trie<T>::remove(const QStringList &key, const T &value) {
#if defined(TRIE_DEBUG)
    qDebug() << "Trie::" << __FUNCTION__ << key << value;
#endif
    Trie<T> *node = walkTo(key, true);
    if (node) {
        bool removed = node->values.removeOne(value);
        if (!removed)
            return false;

        // A faster implementation of removing nodes up the tree
        // can be created if profile shows this to be slow
        QStringList subKey = key;
        while (node->values.isEmpty()
               && node->children.isEmpty()
               && !subKey.isEmpty()) {
            QString currentLevelKey = subKey.first();
            QStringList parentKey = subKey.mid(1);
            Trie<T> *parent = walkTo(parentKey, false);
            Q_ASSERT(parent);
            QStringList::iterator iterator;
            iterator = qBinaryFind(parent->childrenKeys.begin(),
                                   parent->childrenKeys.end(),
                                   currentLevelKey);
            Q_ASSERT(iterator != parent->childrenKeys.end());
            int index = iterator - parent->childrenKeys.begin();
            parent->children.removeAt(index);
            parent->childrenKeys.removeAt(index);

            node = parent;
            subKey = parentKey;
        }
        return removed;
    }
    return false;
}

template<class T>
QList<T> Trie<T>::find(const QStringList &key) const {
#if defined(TRIE_DEBUG)
    qDebug() << "Trie::" << __FUNCTION__ << key;
#endif
    const Trie<T> *node = walkTo(key);
    if (node)
        return node->values;
    return QList<T>();
}

template<class T>
QList<T> Trie<T>::all() const {
#if defined(TRIE_DEBUG)
    qDebug() << "Trie::" << __FUNCTION__;
#endif
    QList<T> all = values;
    for (int i = 0; i < children.count(); ++i)
        all += children[i].all();
    return all;
}

template<class T>
QDataStream &operator<<(QDataStream &out, const Trie<T>&trie) {
    out << trie.values;
    out << trie.childrenKeys;
    out << trie.children;
    Q_ASSERT(trie.childrenKeys.count() == trie.children.count());
    return out;
}

template<class T>
QDataStream &operator>>(QDataStream &in, Trie<T> &trie) {
    trie.clear();
    if (in.status() != QDataStream::Ok)
        return in;
    in >> trie.values;
    in >> trie.childrenKeys;
	in >> trie.children;
    //Q_ASSERT(trie.childrenKeys.count() == trie.children.count());
    if (trie.childrenKeys.count() != trie.children.count())
        in.setStatus(QDataStream::ReadCorruptData);
    return in;
}

// Very fast const walk
template<class T>
const Trie<T>* Trie<T>::walkTo(const QStringList &key) const {
    const Trie<T> *node = this;
    QStringList::const_iterator childIterator;
    QStringList::const_iterator begin, end;

    int depth = key.count() - 1;
    while (depth >= 0) {
        const QString currentLevelKey = key.at(depth--);
        begin = node->childrenKeys.constBegin();
        end = node->childrenKeys.constEnd();
        childIterator = qBinaryFind(begin, end, currentLevelKey);
        if (childIterator == end)
            return 0;
        node = &node->children.at(childIterator - begin);
    }
    return node;
}

template<class T>
Trie<T>* Trie<T>::walkTo(const QStringList &key, bool create) {
    QStringList::iterator iterator;
    Trie<T> *node = this;
    QStringList::iterator begin, end;
    int depth = key.count() - 1;
    while (depth >= 0) {
        const QString currentLevelKey = key.at(depth--);
        begin = node->childrenKeys.begin();
        end = node->childrenKeys.end();
        iterator = qBinaryFind(begin, end, currentLevelKey);
#if defined(TRIE_DEBUG)
        qDebug() << "\t" << node << key << currentLevelKey << node->childrenKeys;
#endif
        int index = -1;
        if (iterator == end) {
            if (!create)
                return 0;
            iterator = qLowerBound(begin,
                                   end,
                                   currentLevelKey);
            index = iterator - begin;
            node->childrenKeys.insert(iterator, currentLevelKey);
            node->children.insert(index, Trie<T>());
        } else {
            index = iterator - begin;
        }
        Q_ASSERT(index >= 0 && index < node->children.count());
        node = &node->children[index];
    }
    return node;
}

#endif
