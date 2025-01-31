/* Copyright (c) <2003-2021> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __D_LIST_H__
#define __D_LIST_H__

#include "dCoreStdafx.h"
#include "dTypes.h"
#include "dMemory.h"
#include "dContainersAlloc.h"

template<class T, class allocator = dContainersAlloc<T> >
class dList
{
	public:
	class dNode: public allocator
	{
		dNode (dNode* const prev, dNode* const next) 
			:allocator()
			,m_info () 
			,m_next(next)
			,m_prev(prev)
		{
			if (m_prev) 
			{
				m_prev->m_next = this;
			}
			if (m_next) 
			{
				m_next->m_prev = this;
			}
		}

		dNode (const T &info, dNode* const prev, dNode* const next) 
			:allocator()
			,m_info (info) 
			,m_next(next)
			,m_prev(prev)
		{
			if (m_prev) 
			{
				m_prev->m_next = this;
			}
			if (m_next) 
			{
				m_next->m_prev = this;
			}
		}

		~dNode()
		{
		}

		void Unlink ()
		{
			if (m_prev) 
			{
				m_prev->m_next = m_next;
			}
			if (m_next) 
			{
				m_next->m_prev = m_prev;
			}
			m_prev = nullptr;
			m_next = nullptr;
		}

		void AddLast(dNode* const node) 
		{
			m_next = node;
			node->m_prev = this;
		}

		void AddFirst(dNode* const node) 
		{
			m_prev = node;
			node->m_next = this;
		}

		public:
		T& GetInfo()
		{
			return m_info;
		}

		dNode *GetNext() const
		{
			return m_next;
		}

		dNode *GetPrev() const
		{
			return m_prev;
		}

		private:
		T m_info;
		dNode *m_next;
		dNode *m_prev;
		friend class dList<T, allocator>;
	};

	class Iterator
	{
		public:
		Iterator (const dList<T, allocator> &me)
		{
			m_ptr = nullptr;
			m_list = (dList *)&me;
		}

		~Iterator ()
		{
		}

		operator dInt32() const
		{
			return m_ptr != nullptr;
		}

		bool operator== (const Iterator &target) const
		{
			return (m_ptr == target.m_ptr) && (m_list == target.m_list);
		}

		void Begin()
		{
			m_ptr = m_list->GetFirst();
		}

		void End()
		{
			m_ptr = m_list->GetLast();
		}

		void Set (dNode* const node)
		{
			m_ptr = node;
		}

		void operator++ ()
		{
			dAssert (m_ptr);
			m_ptr = m_ptr->m_next();
		}

		void operator++ (dInt32)
		{
			dAssert (m_ptr);
			m_ptr = m_ptr->GetNext();
		}

		void operator-- () 
		{
			dAssert (m_ptr);
			m_ptr = m_ptr->GetPrev();
		}

		void operator-- (dInt32) 
		{
			dAssert (m_ptr);
			m_ptr = m_ptr->GetPrev();
		}

		T &operator* () const
		{
			return m_ptr->GetInfo();
		}

		dNode *GetNode() const
		{
			return m_ptr;
		}

		private:
		dList *m_list;
		dNode *m_ptr;
	};

	// ***********************************************************
	// member functions
	// ***********************************************************
	public:
	dList ();
	~dList ();

	operator dInt32() const;
	dInt32 GetCount() const;
	dNode* GetLast() const;
	dNode* GetFirst() const;
	dNode* Append ();
	dNode* Append (dNode* const node);
	dNode* Append (const T &element);
	dNode* Addtop ();
	dNode* Addtop (dNode* const node);
	dNode* Addtop (const T &element);
	
	void RotateToEnd (dNode* const node);
	void RotateToBegin (dNode* const node);
	void InsertAfter (dNode* const root, dNode* const node);
	void InsertBefore (dNode* const root, dNode* const node);

	dNode* Find (const T &element) const;
	dNode* GetNodeFromInfo (T &m_info) const;
	void Remove (dNode* const node);
	void Remove (const T &element);
	void RemoveAll ();

	void Merge (dList<T, allocator>& list);
	void Unlink (dNode* const node);
	bool SanityCheck () const;

	static void FlushFreeList()
	{
		allocator::FlushFreeList();
	}

	protected:
	// ***********************************************************
	// member variables
	// ***********************************************************
	private:
	dNode* m_first;
	dNode* m_last;
	dInt32 m_count;
	friend class dNode;
};

template<class T, class allocator>
dList<T,allocator>::dList ()
	:m_first(nullptr)
	,m_last(nullptr)
	,m_count(0)
{
}

template<class T, class allocator>
dList<T,allocator>::~dList () 
{
	RemoveAll ();
}

template<class T, class allocator>
dInt32 dList<T,allocator>::GetCount() const
{
	return m_count;
}

template<class T, class allocator>
dList<T,allocator>::operator dInt32() const
{
	return m_first != nullptr;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::GetFirst() const
{
	return m_first;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::GetLast() const
{
	return m_last;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::Append (dNode* const node)
{
	dAssert (node->m_next == nullptr);
	dAssert (node->m_prev == nullptr);
	m_count	++;
	if (m_first == nullptr) 
	{
		m_last = node;
		m_first = node;
	} 
	else 
	{
		m_last->AddLast (node);
		m_last = node;
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
	return m_last;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::Append ()
{
	m_count	++;
	if (m_first == nullptr) 
	{
		m_first = new dNode(nullptr, nullptr);
		m_last = m_first;
	} 
	else 
	{
		m_last = new dNode(m_last, nullptr);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
	return m_last;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::Append (const T &element)
{
	m_count	++;
	if (m_first == nullptr) 
	{
		m_first = new dNode(element, nullptr, nullptr);
		m_last = m_first;
	} 
	else 
	{
		m_last = new dNode(element, m_last, nullptr);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif

	return m_last;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::Addtop (dNode* const node)
{
	dAssert (node->m_next == nullptr);
	dAssert (node->m_prev == nullptr);
	m_count	++;
	if (m_last == nullptr) 
	{
		m_last = node;
		m_first = node;
	} 
	else 
	{
		m_first->AddFirst(node);
		m_first = node;
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
	return m_first;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::Addtop ()
{
	m_count	++;
	if (m_last == nullptr) 
	{
		m_last = new dNode(nullptr, nullptr);
		m_first = m_last;
	} 
	else 
	{
		m_first = new dNode(nullptr, m_first);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
	return m_first;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::Addtop (const T &element)
{
	m_count	++;
	if (m_last == nullptr) 
	{
		m_last = new dNode(element, nullptr, nullptr);
		m_first = m_last;
	} 
	else 
	{
		m_first = new dNode(element, nullptr, m_first);
	}
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
	return m_first;
}

template<class T, class allocator>
void dList<T,allocator>::InsertAfter (dNode* const root, dNode* const node)
{
	dAssert (root);
	if (node != root) 
	{
		if (root->m_next != node) 
		{
			if (node == m_first) 
			{
				m_first = node->m_next;
			}
			if (node == m_last) 
			{
				m_last = node->m_prev;
			}
			node->Unlink ();
		
			node->m_prev = root;
			node->m_next = root->m_next;
			if (root->m_next) 
			{
				root->m_next->m_prev = node;
			} 
			root->m_next = node;

			if (node->m_next == nullptr) 
			{
				m_last = node;
			}

			dAssert (m_last);
			dAssert (!m_last->m_next);
			dAssert (m_first);
			dAssert (!m_first->m_prev);
			dAssert (SanityCheck ());
		}
	}
}

template<class T, class allocator>
void dList<T,allocator>::InsertBefore (dNode* const root, dNode* const node)
{
	dAssert (root);
	if (node != root) 
	{
		if (root->m_prev != node) 
		{
			if (node == m_last) 
			{
				m_last = node->m_prev;
			}
			if (node == m_first) {
				m_first = node->m_next;
			}
			node->Unlink ();
		
			node->m_next = root;
			node->m_prev = root->m_prev;
			if (root->m_prev) 
			{
				root->m_prev->m_next = node;
			} 
			root->m_prev = node;

			if (node->m_prev == nullptr) {
				m_first = node;
			}

			dAssert (m_first);
			dAssert (!m_first->m_prev);
			dAssert (m_last);
			dAssert (!m_last->m_next);
			dAssert (SanityCheck ());
		}
	}
}

template<class T, class allocator>
void dList<T,allocator>::RotateToEnd (dNode* const node)
{
	if (node != m_last) 
	{
		if (m_last != m_first) 
		{
			if (node == m_first) 
			{
				m_first = m_first->GetNext();
			}
			node->Unlink();
			m_last->AddLast(node);
			m_last = node; 
		}
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
}

template<class T, class allocator>
void dList<T,allocator>::RotateToBegin (dNode* const node)
{
	if (node != m_first) 
	{
		if (m_last != m_first) 
		{
			if (node == m_last) 
			{
				m_last = m_last->GetPrev();
			}
			node->Unlink();
			m_first->AddFirst(node);
			m_first = node; 
		}
	}

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::Find (const T &element) const
{
	dNode *node;
	for (node = m_first; node; node = node->GetNext()) 
	{
		if (element	== node->m_info) 
		{
			break;
		}
	}
	return node;
}

template<class T, class allocator>
typename dList<T,allocator>::dNode *dList<T,allocator>::GetNodeFromInfo (T &info) const
{
	dNode* const node = (dNode *) &info;
	dInt64 offset = ((char*) &node->m_info) - ((char *) node);
	dNode* const retnode = (dNode *) (((char *) node) - offset);

	dAssert (&retnode->GetInfo () == &info);
	return retnode;
}

template<class T, class allocator> 
void dList<T,allocator>::Remove (const T &element)
{
	dNode *const node = Find (element);
	if (node) 
	{
		Remove (node);
	}
}

template<class T, class allocator> 
void dList<T,allocator>::Unlink (dNode* const node)
{
	dAssert (node);

	m_count --;
	dAssert (m_count >= 0);

	if (node == m_first) 
	{
		m_first = m_first->GetNext();
	}
	if (node == m_last) 
	{
		m_last = m_last->GetPrev();
	}
	node->Unlink();

#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
}

template<class T, class allocator> 
void dList<T,allocator>::Merge (dList<T, allocator>& list)
{
	m_count += list.m_count;
	if (list.m_first) 
	{
		list.m_first->m_prev = m_last; 
	}
	if (m_last) 
	{
		m_last->m_next = list.m_first;
	}
	m_last = list.m_last;
	if (!m_first) 
	{
		m_first = list.m_first;
	}

	list.m_count = 0;
	list.m_last = nullptr;
	list.m_first = nullptr;
#ifdef __ENABLE_DG_CONTAINERS_SANITY_CHECK 
	dAssert (SanityCheck ());
#endif
}

template<class T, class allocator>
void dList<T,allocator>::Remove (dNode* const node)
{
	Unlink (node);
	delete node;
}

template<class T, class allocator>
void dList<T,allocator>::RemoveAll ()
{
	for (dNode *node = m_first; node; node = m_first) 
	{
		m_count --;
		m_first = node->GetNext();
		node->Unlink();
		delete node;
	}
	dAssert (m_count == 0);
	m_last = nullptr;
	m_first = nullptr;
}

template<class T, class allocator>
bool dList<T,allocator>::SanityCheck () const
{
	#ifdef _DEBUG
	dInt32 tCount = 0;
	for (dNode * node = m_first; node; node = node->GetNext()) 
	{
		tCount ++;
		if (node->GetPrev()) {
			dAssert (node->GetPrev() != node->GetNext());
			if (node->GetPrev()->GetNext() != node) 
			{
				dAssert (0);
				return false; 
			}
		}
		if (node->GetNext()) 
		{
			dAssert (node->GetPrev() != node->GetNext());
			if (node->GetNext()->GetPrev() != node)	
			{
				dAssert (0);
				return false;
			}
		}
	}
	if (tCount != m_count) 
	{
		dAssert (0);
		return false;
	}
	#endif
	return true;
}

#endif


