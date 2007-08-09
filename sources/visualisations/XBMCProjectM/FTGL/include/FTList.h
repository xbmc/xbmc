#ifndef    __FTList__
#define    __FTList__

#include "FTGL.h"

/**
* Provides a non-STL alternative to the STL list
 */
template <typename FT_LIST_ITEM_TYPE>
class FTGL_EXPORT FTList
{
    public:
        typedef FT_LIST_ITEM_TYPE value_type;
        typedef value_type& reference;
        typedef const value_type& const_reference;
        typedef size_t size_type;

        /**
         * Constructor
         */
        FTList()
        :   listSize(0),
            tail(0)
        {
            tail = NULL;
            head = new Node;
        }

        /**
         * Destructor
         */
        ~FTList()
        {
            Node* next;
            
            for( Node *walk = head; walk; walk = next)
            {
                next = walk->next;
                delete walk;
            }
        }

        /**
         * Get the number of items in the list
         */
        size_type size() const
        {
            return listSize;
        }

        /**
         * Add an item to the end of the list
         */
        void push_back( const value_type& item)
        {
            Node* node = new Node( item);
            
            if( head->next == NULL)
            {
                head->next = node;
            }

            if( tail)
            {
                tail->next = node;
            }
            tail = node;
            ++listSize;
        }
        
        /**
         * Get the item at the front of the list
         */
        reference front() const
        {
            return head->next->payload;
        }

        /**
         * Get the item at the end of the list
         */
        reference back() const
        {
            return tail->payload;
        }

    private:
        struct Node
        {
            Node()
            :	next(NULL)
            {}

            Node( const value_type& item)
            :	next(NULL)
            {
                payload = item;
            }
            
            Node* next;
            
            value_type payload;
        };
        
        size_type listSize;

        Node* head;
        Node* tail;
};

#endif // __FTList__

