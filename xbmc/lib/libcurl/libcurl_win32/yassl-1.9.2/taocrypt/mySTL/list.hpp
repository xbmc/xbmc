/* mySTL list.hpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL.
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


/* mySTL list implements a simple list
 *
 */

#ifndef mySTL_LIST_HPP
#define mySTL_LIST_HPP


#include "helpers.hpp"


namespace mySTL {



template<typename T> 
class list {

#ifdef __SUNPRO_CC
/*
   Sun Forte 7 C++ v. 5.4 needs class 'node' public to be visible to
   the nested class 'iterator' (a non-standard behaviour).
*/
public:
#endif

    struct node {
        node(T t) : prev_(0), next_(0), value_(t) {}

        node* prev_;
        node* next_;
        T     value_;
    };   
public:
    list() : head_(0), tail_(0), sz_(0) {}
    ~list();

    void   push_front(T);
    void   pop_front();
    T      front() const;
    void   push_back(T);
    void   pop_back();
    T      back() const;
    bool   remove(T);
    size_t size()  const { return sz_; }
    bool   empty() const { return sz_ == 0; }

    class iterator {
        node* current_;
    public:
        explicit iterator(node* p = 0) : current_(p) {}

        T& operator*() const
        {
            return current_->value_;
        }

        T* operator->() const
        {
            return &(operator*());
        }

        iterator& operator++()
        {
            current_ = current_->next_;
            return *this;
        }

        iterator& operator--()
        {
            current_ = current_->prev_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            current_ = current_->next_;
            return tmp;
        }

        iterator operator--(int)
        {
            iterator tmp = *this;
            current_ = current_->prev_;
            return tmp;
        }

        bool operator==(const iterator& other) const
        { 
            return current_ == other.current_;
        }

        bool operator!=(const iterator& other) const
        {
            return current_ != other.current_;
        }

        friend class list<T>;
    };


    class reverse_iterator {
        node* current_;
    public:
        explicit reverse_iterator(node* p = 0) : current_(p) {}

        T& operator*() const
        {
            return current_->value_;
        }

        T* operator->() const
        {
            return &(operator*());
        }

        reverse_iterator& operator++()
        {
            current_ = current_->prev_;
            return *this;
        }

        reverse_iterator& operator--()
        {
            current_ = current_->next_;
            return *this;
        }

        reverse_iterator operator++(int)
        {
            reverse_iterator tmp = *this;
            current_ = current_->prev_;
            return tmp;
        }

        reverse_iterator operator--(int)
        {
            reverse_iterator tmp = *this;
            current_ = current_->next_;
            return tmp;
        }

        bool operator==(const reverse_iterator& other) const
        { 
            return current_ == other.current_;
        }

        bool operator!=(const reverse_iterator& other) const
        {
            return current_ != other.current_;
        }

        friend class list<T>;
    };

    bool erase(iterator);

    iterator         begin()  const { return iterator(head_); }
    reverse_iterator rbegin() const { return reverse_iterator(tail_); }
    iterator         end()    const { return iterator(); }
    reverse_iterator rend()   const { return reverse_iterator(); }

    typedef iterator const_iterator;    // for now

    class underflow {};
    class overflow {}; 
private:
    node*  head_;
    node*  tail_;
    size_t sz_;

    node* look_up(T);

    list(const list&);            // hide copy
    list& operator=(const list&); // and assign
};


template<typename T> 
list<T>::~list()
{
    node* start = head_;
    node* next_;

    for (; start; start = next_) {
        next_ = start->next_;
        destroy(start);
        FreeMemory(start);
    }
}


template<typename T> 
void list<T>::push_front(T t)
{
    void* mem = GetMemory(sizeof(node));
    node* add = new (reinterpret_cast<yassl_pointer>(mem)) node(t);

    if (head_) {
        add->next_ = head_;
        head_->prev_ = add;
    }
    else
        tail_ = add;

    head_ = add;
    ++sz_; 
}


template<typename T> 
void list<T>::pop_front()
{
    node* front = head_;

    if (head_ == 0)
        return;
    else if (head_ == tail_)
        head_ = tail_ = 0;
    else {
        head_ = head_->next_;
        head_->prev_ = 0;
    }
    destroy(front);
    FreeMemory(front);
    --sz_;
}


template<typename T> 
T list<T>::front() const
{
    if (head_ == 0) return T();
    return head_->value_;
}


template<typename T> 
void list<T>::push_back(T t)
{
    void* mem = GetMemory(sizeof(node));
    node* add = new (reinterpret_cast<yassl_pointer>(mem)) node(t);

    if (tail_) {
        tail_->next_ = add;
        add->prev_ = tail_;
    }
    else
        head_ = add;

    tail_ = add;
    ++sz_;
}


template<typename T> 
void list<T>::pop_back()
{
    node* rear = tail_;

    if (tail_ == 0)
        return;
    else if (tail_ == head_)
        tail_ = head_ = 0;
    else {
        tail_ = tail_->prev_;
        tail_->next_ = 0;
    }
    destroy(rear);
    FreeMemory(rear);
    --sz_;
}


template<typename T> 
T list<T>::back() const
{
    if (tail_ == 0) return T();
    return tail_->value_;
}


template<typename T>
typename list<T>::node* list<T>::look_up(T t)
{
    node* list = head_;

    if (list == 0) return 0;

    for (; list; list = list->next_)
        if (list->value_ == t)
            return list;

    return 0;
}


template<typename T> 
bool list<T>::remove(T t)
{
    node* del = look_up(t);

    if (del == 0)
        return false;
    else if (del == head_)
        pop_front();
    else if (del == tail_)
        pop_back();
    else {
        del->prev_->next_ = del->next_;
        del->next_->prev_ = del->prev_;

        destroy(del);
        FreeMemory(del);
        --sz_;
    }
    return true;
}


template<typename T> 
bool list<T>::erase(iterator iter)
{
    node* del = iter.current_;

    if (del == 0)
        return false;
    else if (del == head_)
        pop_front();
    else if (del == tail_)
        pop_back();
    else {
        del->prev_->next_ = del->next_;
        del->next_->prev_ = del->prev_;

        destroy(del);
        FreeMemory(del);
        --sz_;
    }
    return true;
}



} // namespace mySTL

#endif // mySTL_LIST_HPP
