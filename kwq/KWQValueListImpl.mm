/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "KWQValueListImpl.h"

#import <stdlib.h>

KWQValueListNodeImpl::KWQValueListNodeImpl() : 
    prev(NULL), 
    next(NULL)
{
}

KWQValueListIteratorImpl::KWQValueListIteratorImpl() : 
    nodeImpl(NULL)
{
}

bool KWQValueListIteratorImpl::operator==(const KWQValueListIteratorImpl &other)
{
    return nodeImpl == other.nodeImpl;
}

bool KWQValueListIteratorImpl::operator!=(const KWQValueListIteratorImpl &other)
{
    return nodeImpl != other.nodeImpl;
}

KWQValueListNodeImpl *KWQValueListIteratorImpl::node()
{
    return nodeImpl;
}

const KWQValueListNodeImpl *KWQValueListIteratorImpl::node() const
{
    return nodeImpl;
}

KWQValueListIteratorImpl& KWQValueListIteratorImpl::operator++()
{
    if (nodeImpl != NULL) {
	nodeImpl = nodeImpl->next;
    }
    return *this;
}

KWQValueListIteratorImpl KWQValueListIteratorImpl::operator++(int)
{
    KWQValueListIteratorImpl tmp(*this);

    if (nodeImpl != NULL) {
	nodeImpl = nodeImpl->next;
    }

    return tmp;
}

KWQValueListIteratorImpl& KWQValueListIteratorImpl::operator--()
{
    if (nodeImpl != NULL) {
	nodeImpl = nodeImpl->prev;
    }
    return *this;
}

KWQValueListIteratorImpl::KWQValueListIteratorImpl(const KWQValueListNodeImpl *n) :
    nodeImpl((KWQValueListNodeImpl *)n)
{
}


class KWQValueListImpl::KWQValueListPrivate
{
public:
    KWQValueListPrivate(void (*deleteFunc)(KWQValueListNodeImpl *), KWQValueListNodeImpl *(*copyFunc)(KWQValueListNodeImpl *));
    KWQValueListPrivate(const KWQValueListPrivate &other);

    ~KWQValueListPrivate();

    KWQValueListNodeImpl *copyList(KWQValueListNodeImpl *l) const;
    void deleteList(KWQValueListNodeImpl *l);

    KWQValueListNodeImpl *head;

    void (*deleteNode)(KWQValueListNodeImpl *);
    KWQValueListNodeImpl *(*copyNode)(KWQValueListNodeImpl *);
    uint count;

    uint refCount;
};

KWQValueListImpl::KWQValueListPrivate::KWQValueListPrivate(void (*deleteFunc)(KWQValueListNodeImpl *), 
							   KWQValueListNodeImpl *(*copyFunc)(KWQValueListNodeImpl *)) : 
    head(NULL),
    deleteNode(deleteFunc),
    copyNode(copyFunc),
    count(0),
    refCount(0)
{
}

KWQValueListImpl::KWQValueListPrivate::KWQValueListPrivate(const KWQValueListPrivate &other) :
    head(other.copyList(other.head)),
    deleteNode(other.deleteNode),
    copyNode(other.copyNode),
    count(other.count),
    refCount(0)
{
}

KWQValueListImpl::KWQValueListPrivate::~KWQValueListPrivate()
{
    deleteList(head);
}

KWQValueListNodeImpl *KWQValueListImpl::KWQValueListPrivate::copyList(KWQValueListNodeImpl *l) const
{
    KWQValueListNodeImpl *prev = NULL;
    KWQValueListNodeImpl *node = l;
    KWQValueListNodeImpl *head = NULL;

    while (node != NULL) {
	KWQValueListNodeImpl *copy = copyNode(node);
	if (prev == NULL) {
	    head = copy;
	} else {
	    prev->next = copy;
	}

	copy->prev = prev;
	copy->next = NULL;

	prev = copy;
	node = node->next;
    }

    return head;
}

void KWQValueListImpl::KWQValueListPrivate::deleteList(KWQValueListNodeImpl *l)
{
    KWQValueListNodeImpl *p = l;
    
    while (p != NULL) {
	KWQValueListNodeImpl *next = p->next;
	deleteNode(p);
	p = next;
    }
}

KWQValueListImpl::KWQValueListImpl(void (*deleteFunc)(KWQValueListNodeImpl *), KWQValueListNodeImpl *(*copyFunc)(KWQValueListNodeImpl *)) :
    d(new KWQValueListPrivate(deleteFunc, copyFunc))
{
}

KWQValueListImpl::KWQValueListImpl(const KWQValueListImpl &other) :
    d(other.d)
{
}

KWQValueListImpl::~KWQValueListImpl()
{
}

void KWQValueListImpl::clear()
{
    if (d->head) {
        copyOnWrite();
        d->deleteList(d->head);
        d->head = NULL;
        d->count = 0;
    }
}

uint KWQValueListImpl::count() const
{
    return d->count;
}

bool KWQValueListImpl::isEmpty() const
{
    return d->count == 0;
}

KWQValueListIteratorImpl KWQValueListImpl::appendNode(KWQValueListNodeImpl *node)
{
    copyOnWrite();

    // FIXME: maintain tail pointer to make this fast
    
    if (d->head == NULL) {
	d->head = node;
    } else {
	KWQValueListNodeImpl *p = d->head;

	while (p->next != NULL) {
	    p = p->next;
	}

	p->next = node;
	node->prev = p;
	node->next = NULL;
    }

    d->count++;
    
    return node;
}

KWQValueListIteratorImpl KWQValueListImpl::prependNode(KWQValueListNodeImpl *node)
{
    copyOnWrite();

    node->next = d->head;
    node->prev = NULL;
    d->head = node;

    if (node->next != NULL) {
	node->next->prev = node;
    }

    d->count++;
    
    return node;
}

void KWQValueListImpl::removeEqualNodes(KWQValueListNodeImpl *node, bool (*equalFunc)(const KWQValueListNodeImpl *, const KWQValueListNodeImpl *))
{
    copyOnWrite();

    KWQValueListNodeImpl *p = d->head;
    
    while (p != NULL) {
	KWQValueListNodeImpl *next = p->next;
	if (equalFunc(node, p)) {
	    if (p->next != NULL) {
		p->next->prev = p->prev;
	    }

	    if (p->prev != NULL) {
		p->prev->next = p->next;
	    } else {
		d->head = p->next;
	    }

	    d->deleteNode(p);

	    d->count--;
	}
	p = next;
    }
}

uint KWQValueListImpl::containsEqualNodes(KWQValueListNodeImpl *node, bool (*equalFunc)(const KWQValueListNodeImpl *, const KWQValueListNodeImpl *)) const
{
    KWQValueListNodeImpl *p = d->head;
    unsigned contains = 0;

    while (p != NULL) {
	if (equalFunc(node, p)) {
	    ++contains;
	}
	p = p->next;
    }
    
    return contains;
}

KWQValueListIteratorImpl KWQValueListImpl::insert(const KWQValueListIteratorImpl &iterator, KWQValueListNodeImpl *node)
{
    copyOnWrite();
    
    KWQValueListNodeImpl *next = iterator.nodeImpl;
    
    if (next == NULL)
        return appendNode(node);
    
    if (next == d->head)
        return prependNode(node);
    
    KWQValueListNodeImpl *prev = next->prev;
    
    node->next = next;
    node->prev = prev;
    next->prev = node;
    prev->next = node;
    
    d->count++;
    
    return node;
}

KWQValueListIteratorImpl KWQValueListImpl::removeIterator(KWQValueListIteratorImpl &iterator)
{
    copyOnWrite();

    if (iterator.nodeImpl == NULL) {
	return iterator;
    }

    KWQValueListNodeImpl *next = iterator.nodeImpl->next;

    // detach node
    if (iterator.nodeImpl->next != NULL) {
	iterator.nodeImpl->next->prev = iterator.nodeImpl->prev;
    }
    if (iterator.nodeImpl->prev != NULL) {
	iterator.nodeImpl->prev->next = iterator.nodeImpl->next;
    } else {
	d->head = iterator.nodeImpl->next;
    }
    
    d->deleteNode(iterator.nodeImpl);
    d->count--;

    return KWQValueListIteratorImpl(next);
}

KWQValueListIteratorImpl KWQValueListImpl::fromLast()
{
    copyOnWrite();
    return KWQValueListIteratorImpl(lastNode());
}

KWQValueListNodeImpl *KWQValueListImpl::firstNode()
{
    copyOnWrite();
    return ((const KWQValueListImpl *)this)->firstNode();
}

KWQValueListNodeImpl *KWQValueListImpl::lastNode()
{
    copyOnWrite();
    return ((const KWQValueListImpl *)this)->lastNode();
}

KWQValueListNodeImpl *KWQValueListImpl::firstNode() const
{
    return d->head;
}

KWQValueListNodeImpl *KWQValueListImpl::lastNode() const
{
    // FIXME: use tail pointer

    KWQValueListNodeImpl *p = d->head;

    if (p == NULL) {
	return NULL;
    }
    
    while (p->next != NULL) {
	p = p->next;
    }

    return p;
}

KWQValueListIteratorImpl KWQValueListImpl::begin()
{
    copyOnWrite();
    return ((const KWQValueListImpl *)this)->begin();
}

KWQValueListIteratorImpl KWQValueListImpl::end()
{
    copyOnWrite();
    return ((const KWQValueListImpl *)this)->end();
}


KWQValueListIteratorImpl KWQValueListImpl::begin() const
{
    return KWQValueListIteratorImpl(firstNode());
}

KWQValueListIteratorImpl KWQValueListImpl::end() const
{
    return KWQValueListIteratorImpl(NULL);
}

KWQValueListIteratorImpl KWQValueListImpl::fromLast() const
{
    return KWQValueListIteratorImpl(lastNode());
}

KWQValueListNodeImpl *KWQValueListImpl::nodeAt(uint index)
{
    copyOnWrite();

    if (d->count <= index) {
	return NULL;
    }

    KWQValueListNodeImpl *p = d->head;

    for (uint i = 0; i < index; i++) {
	p = p->next;
    }

    return p;
}

KWQValueListNodeImpl *KWQValueListImpl::nodeAt(uint index) const
{
    if (d->count <= index) {
	return NULL;
    }

    KWQValueListNodeImpl *p = d->head;

    for (uint i = 0; i < index; i++) {
	p = p->next;
    }

    return p;
}

KWQValueListImpl& KWQValueListImpl::operator=(const KWQValueListImpl &other)
{
    KWQValueListImpl tmp(other);
    KWQRefPtr<KWQValueListPrivate> tmpD = tmp.d;

    tmp.d = d;
    d = tmpD;

    return *this;
}

void KWQValueListImpl::copyOnWrite()
{
    if (d->refCount > 1) {
	d = KWQRefPtr<KWQValueListPrivate>(new KWQValueListPrivate(*d));
    }
}

bool KWQValueListImpl::isEqual(const KWQValueListImpl &other, bool (*equalFunc)(const KWQValueListNodeImpl *, const KWQValueListNodeImpl *)) const
{
    KWQValueListNodeImpl *p, *q;
    for (p = d->head, q = other.d->head; p && q; p = p->next, q = q->next) {
	if (!equalFunc(p, q)) {
	    return false;
	}
    }
    return !p && !q;
}
