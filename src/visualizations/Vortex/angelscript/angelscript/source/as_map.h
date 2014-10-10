/*
   AngelCode Scripting Library
   Copyright (c) 2003-2007 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_map.h
//
// This class is used for mapping a value to another
//


#ifndef AS_MAP_H
#define AS_MAP_H

template <class KEY, class VAL> struct asSMapNode;

template <class KEY, class VAL> class asCMap
{
public:
	asCMap();
	~asCMap();

	int   Insert(const KEY &key, const VAL &value);
	int   GetCount();
	
	const KEY &GetKey(const asSMapNode<KEY,VAL> *cursor) const;
	const VAL &GetValue(const asSMapNode<KEY,VAL> *cursor) const;
	VAL &GetValue(asSMapNode<KEY,VAL> *cursor);

	void Erase(asSMapNode<KEY,VAL> *cursor);
	void EraseAll();

	// Returns true as long as cursor is valid

	bool MoveTo(asSMapNode<KEY,VAL> **out, const KEY &key);
	bool MoveFirst(asSMapNode<KEY,VAL> **out);
	bool MoveLast(asSMapNode<KEY,VAL> **out);
	bool MoveNext(asSMapNode<KEY,VAL> **out, asSMapNode<KEY,VAL> *cursor);
	bool MovePrev(asSMapNode<KEY,VAL> **out, asSMapNode<KEY,VAL> *cursor);

	// For debugging only

	int CheckIntegrity(asSMapNode<KEY,VAL> *node);

protected:
	void BalanceInsert(asSMapNode<KEY,VAL> *node);
	void BalanceErase(asSMapNode<KEY,VAL> *child, asSMapNode<KEY,VAL> *parent);

	int EraseAll(asSMapNode<KEY,VAL> *node);
	int RotateLeft(asSMapNode<KEY,VAL> *node);
	int RotateRight(asSMapNode<KEY,VAL> *node);

	asSMapNode<KEY,VAL> *root;
	asSMapNode<KEY,VAL> dummy;

	int count;
};

//---------------------------------------------------------------------------
// Implementation

// Properties of a Red-Black Tree
//
// 1. The root is always black
// 2. All single paths from the root to leafs
//    contain the same amount of black nodes
// 3. No red node can have a red node as parent

#define ISRED(x) ((x != 0) && (x)->isRed)
#define ISBLACK(x) (!ISRED(x))

template <class KEY, class VAL> struct asSMapNode
{
	asSMapNode() {parent = 0; left = 0; right = 0; isRed = true;}

	asSMapNode *parent;
	asSMapNode *left;
	asSMapNode *right;
	bool isRed;

	KEY key;
	VAL value;
};

template <class KEY, class VAL>
asCMap<KEY, VAL>::asCMap()
{
	root = 0;
	count = 0;
}

template <class KEY, class VAL>
asCMap<KEY, VAL>::~asCMap()
{
	EraseAll();
}

template <class KEY, class VAL>
void asCMap<KEY, VAL>::EraseAll()
{
	EraseAll(root);
	root = 0;
}

template <class KEY, class VAL>
int asCMap<KEY, VAL>::EraseAll(asSMapNode<KEY, VAL> *p)
{
	if( p == 0 ) return -1;

	EraseAll( p->left );
	EraseAll( p->right );

	typedef asSMapNode<KEY,VAL> node_t;
	asDELETE(p,node_t);

	count--;

	return 0;
}

template <class KEY, class VAL>
int asCMap<KEY, VAL>::GetCount()
{
	return count;
}

template <class KEY, class VAL>
int asCMap<KEY, VAL>::Insert(const KEY &key, const VAL &value)
{
	typedef asSMapNode<KEY,VAL> node_t;
	asSMapNode<KEY,VAL> *nnode = asNEW(node_t);
	nnode->key   = key;
	nnode->value = value;

	// Insert the node
	if( root == 0 )
		root = nnode;
	else
	{
		asSMapNode<KEY,VAL> *p = root;
		for(;;)
		{
			if( nnode->key < p->key )
			{
				if( p->left == 0 )
				{
					nnode->parent = p;
					p->left = nnode;
					break;
				}
				else
					p = p->left;
			}
			else
			{
				if( p->right == 0 )
				{
					nnode->parent = p;
					p->right = nnode;
					break;
				}
				else
					p = p->right;
			}
		}
	}

	BalanceInsert(nnode);

	count++;

	return 0;
}

template <class KEY, class VAL>
void asCMap<KEY, VAL>::BalanceInsert(asSMapNode<KEY, VAL> *node)
{
	// The node, that is red, can't have a red parent
	while( node != root && node->parent->isRed )
	{
		// Check color of uncle 
		if( node->parent == node->parent->parent->left )
		{
			asSMapNode<KEY,VAL> *uncle = node->parent->parent->right;
			if( ISRED(uncle) )
			{
				//    B
				//   R R
				//  N

				// Change color on parent, uncle, and grand parent
				node->parent->isRed = false;
				uncle->isRed = false;
				node->parent->parent->isRed = true;

				// Continue balancing from grand parent
				node = node->parent->parent;
			}
			else
			{
				//    B
				//   R B
				//  N

				if( node == node->parent->right ) 
				{
                    // Make the node a left child
                    node = node->parent;
                    RotateLeft(node);
                }

				// Change color on parent and grand parent
				// Then rotate grand parent to the right
				node->parent->isRed = false;
				node->parent->parent->isRed = true;
				RotateRight(node->parent->parent);
			}
		}
		else
		{
			asSMapNode<KEY,VAL> *uncle = node->parent->parent->left;
			if( ISRED(uncle) )
			{
				//   B
				//  R R
				//     N

				// Change color on parent, uncle, and grand parent
				// Continue balancing from grand parent
				node->parent->isRed = false;
				uncle->isRed = false;
				node = node->parent->parent;
				node->isRed = true;
			}
			else
			{
				//   B
				//  B R
				//     N

				if( node == node->parent->left ) 
				{
                    // Make the node a right child
                    node = node->parent;
                    RotateRight(node);
                }
				
				// Change color on parent and grand parent
				// Then rotate grand parent to the right
				node->parent->isRed = false;
				node->parent->parent->isRed = true;
				RotateLeft(node->parent->parent);
			}
		}
	}

	root->isRed = false;
}

// For debugging purposes only
template <class KEY, class VAL>
int asCMap<KEY, VAL>::CheckIntegrity(asSMapNode<KEY, VAL> *node)
{
	if( node == 0 ) 
	{
		if( root == 0 ) 
			return 0;
		else if( ISRED(root) )
			return -1;
		else
			node = root;
	}

	int left = 0, right = 0;
	if( node->left )
		left = CheckIntegrity(node->left);
	if( node->right )
		right = CheckIntegrity(node->right);

	if( left != right || left == -1 ) 
		return -1;
	
	if( ISBLACK(node) )
		return left+1;

	return left;
}

// Returns true if successful
template <class KEY, class VAL>
bool asCMap<KEY, VAL>::MoveTo(asSMapNode<KEY,VAL> **out, const KEY &key)
{
	asSMapNode<KEY,VAL> *p = root;
	while( p )
	{
		if( key < p->key )
			p = p->left;
		else if( key == p->key )
		{
			*out = p;
			return true;
		}
		else 
			p = p->right;
	}

	*out = 0;
	return false;
}

template <class KEY, class VAL>
void asCMap<KEY, VAL>::Erase(asSMapNode<KEY,VAL> *cursor)
{
	if( cursor == 0 ) return;

	asSMapNode<KEY,VAL> *node = cursor;

	//---------------------------------------------------
	// Choose the node that will replace the erased one
	asSMapNode<KEY,VAL> *remove;
	if( node->left == 0 || node->right == 0 )
		remove = node;
	else
	{
		remove = node->right;
		while( remove->left ) remove = remove->left;
	}

	//--------------------------------------------------
	// Remove the node
	asSMapNode<KEY,VAL> *child;
	if( remove->left )
		child = remove->left;
	else
		child = remove->right;

	if( child ) child->parent = remove->parent;
    if( remove->parent )
	{
        if( remove == remove->parent->left )
            remove->parent->left = child;
        else
            remove->parent->right = child;
	}
	else
        root = child;

	// If we remove a black node we must make sure the tree is balanced
	if( ISBLACK(remove) )
		BalanceErase(child, remove->parent);

	//----------------------------------------
	// Replace the erased node with the removed one
	if( remove != node )
	{
		if( node->parent )
		{
			if( node->parent->left == node )
				node->parent->left = remove;
			else
				node->parent->right = remove;
		}
		else
			root = remove;

		remove->isRed  = node->isRed;
		remove->parent = node->parent;

		remove->left = node->left;
		if( remove->left ) remove->left->parent = remove;
		remove->right = node->right;
		if( remove->right ) remove->right->parent = remove;	
	}

	typedef asSMapNode<KEY,VAL> node_t;
	asDELETE(node,node_t);

	count--;
}

// Call method only if removed node was black
// child is the child of the removed node
template <class KEY, class VAL>
void asCMap<KEY, VAL>::BalanceErase(asSMapNode<KEY, VAL> *child, asSMapNode<KEY, VAL> *parent)
{
	// If child is red
	//   Color child black 
	//   Terminate

	// These tests assume brother is to the right.

	// 1. Brother is red
	//   Color parent red and brother black
    //   Rotate parent left
	//   Transforms to 2b
	// 2a. Parent and brother is black, brother's children are black
	//   Color brother red
	//   Continue test with parent as child
	// 2b. Parent is red, brother is black, brother's children are black
	//   Color parent black and brother red
	//   Terminate
	// 3. Brother is black, and brother's left is red and brother's right is black
	//   Color brother red and brother's left black
	//   Rotate brother to right
	//   Transforms to 4.
	// 4. Brother is black, brother's right is red
	//   Color brother's right black
	//   Color brother to color of parent
	//   Color parent black
	//   Rotate parent left
	//   Terminate

	while( child != root && ISBLACK(child) )
	{
		if( child == parent->left )
		{
			asSMapNode<KEY,VAL> *brother = parent->right;

			// Case 1
			if( ISRED(brother) )
			{
				brother->isRed = false;
				parent->isRed = true;
				RotateLeft(parent);
				brother = parent->right;
			}

			// Case 2
			if( brother == 0 ) break;
			if( ISBLACK(brother->left) && ISBLACK(brother->right) )
			{
				// Case 2b
				if( ISRED(parent) )
				{
					parent->isRed = false;
					brother->isRed = true;
					break;
				}

				brother->isRed = true;
				child = parent;
				parent = child->parent;
			}
			else
			{
				// Case 3
				if( ISBLACK(brother->right) )
				{
					brother->left->isRed = false;
                    brother->isRed = true;
                    RotateRight(brother);
                    brother = parent->right;
				}

				// Case 4
				brother->isRed = parent->isRed;
                parent->isRed = false;
                brother->right->isRed = false;
                RotateLeft(parent);
                break;
			}
		}
		else
		{
			asSMapNode<KEY,VAL> *brother = parent->left;
			
			// Case 1
			if( ISRED(brother) )
			{
				brother->isRed = false;
				parent->isRed = true;
				RotateRight(parent);
				brother = parent->left;
			}

			// Case 2
			if( brother == 0 ) break;
			if( ISBLACK(brother->left) && ISBLACK(brother->right) )
			{
				// Case 2b
				if( ISRED(parent) )
				{
					parent->isRed = false;
					brother->isRed = true;
					break;
				}

				brother->isRed = true;
				child = parent;
				parent = child->parent;
			}
			else
			{
				// Case 3
				if( ISBLACK(brother->left) )
				{
					brother->right->isRed = false;
                    brother->isRed = true;
                    RotateLeft(brother);
                    brother = parent->left;
				}

				// Case 4
				brother->isRed = parent->isRed;
                parent->isRed = false;
                brother->left->isRed = false;
                RotateRight(parent);
                break;
			}
		}
	}

	if( child )
		child->isRed = false;
}

template <class KEY, class VAL>
int asCMap<KEY, VAL>::RotateRight(asSMapNode<KEY, VAL> *node)
{
	//     P             L       //
	//    / \           / \      //
	//   L   R    =>   Ll  P     //
	//  / \               / \    //
	// Ll  Lr            Lr  R   //

	if( node->left == 0 ) return -1;

	asSMapNode<KEY,VAL> *left = node->left;

	// Update parent
	if( node->parent )
	{
		asSMapNode<KEY,VAL> *parent = node->parent;
		if( parent->left == node )
			parent->left = left;
		else
			parent->right = left;

		left->parent = parent;
	}
	else
	{
		root = left;
		left->parent = 0;
	}

	// Move left's right child to node's left child
	node->left = left->right;
	if( node->left ) node->left->parent = node;

	// Put node as left's right child
	left->right = node;
	node->parent = left;

	return 0;
}

template <class KEY, class VAL>
int asCMap<KEY, VAL>::RotateLeft(asSMapNode<KEY, VAL> *node)
{
	//     P             R      //
	//    / \           / \     //
	//   L   R    =>   P   Rr   //
	//      / \       / \       //
	//     Rl  Rr    L   Rl     //

	if( node->right == 0 ) return -1;

	asSMapNode<KEY,VAL> *right = node->right;

	// Update parent
	if( node->parent )
	{
		asSMapNode<KEY,VAL> *parent = node->parent;
		if( parent->right == node )
			parent->right = right;
		else
			parent->left = right;

		right->parent = parent;
	}
	else
	{
		root = right;
		right->parent = 0;
	}

	// Move right's left child to node's right child
	node->right = right->left;
	if( node->right ) node->right->parent = node;

	// Put node as right's left child
	right->left = node;
	node->parent = right;

	return 0;
}

template <class KEY, class VAL>
const VAL &asCMap<KEY, VAL>::GetValue(const asSMapNode<KEY,VAL> *cursor) const 
{
	if( cursor == 0 ) 
		return dummy.value;

	return cursor->value;
}

template <class KEY, class VAL>
VAL &asCMap<KEY, VAL>::GetValue(asSMapNode<KEY,VAL> *cursor)
{
	if( cursor == 0 ) 
		return dummy.value;

	return cursor->value;
}

template <class KEY, class VAL>
const KEY &asCMap<KEY, VAL>::GetKey(const asSMapNode<KEY,VAL> *cursor) const
{
	if( cursor == 0 )
		return dummy.key;

	return cursor->key;
}

template <class KEY, class VAL>
bool asCMap<KEY, VAL>::MoveFirst(asSMapNode<KEY,VAL> **out)
{
	*out = root;
	if( root == 0 ) return false;

	while( (*out)->left ) 
		*out = (*out)->left;

	return true;
}

template <class KEY, class VAL>
bool asCMap<KEY, VAL>::MoveLast(asSMapNode<KEY,VAL> **out)
{
	*out = root;
	if( root == 0 ) return false;

	while( (*out)->right ) 
		*out = (*out)->right;

	return true;
}

template <class KEY, class VAL>
bool asCMap<KEY, VAL>::MoveNext(asSMapNode<KEY,VAL> **out, asSMapNode<KEY,VAL> *cursor)
{
	if( cursor == 0 )
	{
		*out = 0;
		return false;
	}

	if( cursor->right == 0 ) 
	{
		// Move upwards until we find a parent node to the right
		while( cursor->parent && cursor->parent->right == cursor )
			cursor = cursor->parent;

		cursor = cursor->parent;
		*out = cursor;
		if( cursor == 0 ) 
			return false;

		return true;
	}

	cursor = cursor->right;
	while( cursor->left ) 
		cursor = cursor->left;

	*out = cursor;
	return true;
}

template <class KEY, class VAL>
bool asCMap<KEY, VAL>::MovePrev(asSMapNode<KEY,VAL> **out, asSMapNode<KEY,VAL> *cursor)
{
	if( cursor == 0 ) 
	{
		*out = 0;
		return false;
	}

	if( cursor->left == 0 )
	{
		// Move upwards until we find a parent node to the left
		while( cursor->parent && cursor->parent->left == cursor )
			cursor = cursor->parent;

		cursor = cursor->parent;

		*out = cursor;
		if( cursor == 0 )
			return false;

		return true;
	}

	cursor = cursor->left;
	while( cursor->right )
		cursor = cursor->right;

	*out = cursor;
	return true;
}




#endif

