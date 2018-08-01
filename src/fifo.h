/*
 * Copyright (C) 2015-2018 UAB Vilniaus Blokas
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license.  See the LICENSE file for details.
 */

#ifndef FIFO_H
#define FIFO_H

#include <string.h>

template <typename T, typename IndexType, const IndexType N>
class TFifo
{
public:
	TFifo();

	bool empty() const;
	bool full() const;
	IndexType size() const;

	bool peek(T &item) const;
	void advance();

	bool pop(T &item);
	void push(T item);

	bool hasSpaceFor(IndexType n) const;

private:
	IndexType next(IndexType i) const;

	T m_items[N];
	IndexType m_front;
	IndexType m_back;
};

template <typename T, typename IndexType, const IndexType N>
inline TFifo<T, IndexType, N>::TFifo()
	:m_front(0)
	,m_back(0)
{
	memset(m_items, 0, sizeof(m_items));
}

template <typename T, typename IndexType, const IndexType N>
inline bool TFifo<T, IndexType, N>::empty() const
{
	return m_front == m_back;
}

template <typename T, typename IndexType, const IndexType N>
inline bool TFifo<T, IndexType, N>::full() const
{
	return m_front == next(m_back);
}

template <typename T, typename IndexType, const IndexType N>
inline IndexType TFifo<T, IndexType, N>::size() const
{
	return (m_back - m_front + N) % N;
}

template <typename T, typename IndexType, const IndexType N>
inline bool TFifo<T, IndexType, N>::peek(T &item) const
{
	if (empty())
		return false;

	item = m_items[m_front];
	return true;
}

template <typename T, typename IndexType, const IndexType N>
inline void TFifo<T, IndexType, N>::advance()
{
	if (empty())
		return;

	m_front = next(m_front);
}

template <typename T, typename IndexType, const IndexType N>
inline bool TFifo<T, IndexType, N>::pop(T &item)
{
	if (!peek(item))
		return false;

	advance();
	return true;
}

template <typename T, typename IndexType, const IndexType N>
inline void TFifo<T, IndexType, N>::push(T item)
{
	if (full())
		return;

	m_items[m_back] = item;
	m_back = next(m_back);
}

template <typename T, typename IndexType, const IndexType N>
inline bool TFifo<T, IndexType, N>::hasSpaceFor(IndexType n) const
{
	return N - ((N + m_back - m_front) % N) >= n;
}

template <typename T, typename IndexType, const IndexType N>
inline IndexType TFifo<T, IndexType, N>::next(IndexType i) const
{
	return (i + 1) % N;
}

#endif // FIFO_H
