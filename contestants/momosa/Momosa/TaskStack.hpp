/*
 * Copyright (c) 2015 Patrick Moore
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>

// Using as replacement for vector in the iterative search. Trades safety for a slight speed increase.
template <typename Type>
class TaskStack
{
public:
    TaskStack() 
        : m_current(0) 
        , m_size(0)
    {}

    TaskStack(TaskStack const& t)
    {
        m_size = t.m_size;
        m_stack = new Type[m_size];
        for(int i=0; i < m_size; ++i)
        {
            m_stack[i] = t.m_stack[i];
        }
    }

    ~TaskStack()
    {
        if(m_size > 0) { delete[] m_stack; }
        m_size = 0;
    }

    void reserve(std::size_t count)
    {
        if(count == 0 || count == m_size) { return; }
        if(m_size > 0) { delete[] m_stack; }
        m_stack = new Type[count];
        m_size = count;
        m_current = 0;
    }

    void clear()
    {
        m_current = 0;
    }

    bool empty()
    {
        return m_current == 0;
    }

    void push_back(Type const& value)
    {
        assert(m_current < m_size);
        m_stack[m_current] = value;
        m_current++;
    }

    void pop_back()
    {
        --m_current;
    }

    Type& back()
    {
        return m_stack[m_current-1];
    }

    std::size_t size() { return m_current; }

    Type* m_stack;
    std::size_t m_current;
    std::size_t m_size;
};