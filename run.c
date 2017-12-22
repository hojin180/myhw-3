#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "run.h"
#include "util.h"

void *base = 0;
void *last = 0;
//int reallocing = 0;

p_meta find_meta(p_meta *last, size_t size) {
  p_meta index = base;
  p_meta result = base;
  //p_meta tmp = NULL;
  //p_meta tmpSize;
  switch(fit_flag){
    case FIRST_FIT:
    { 
      //FIRST FIT CODE
      while(index!=last)
      {
        if(index->size+META_SIZE>=size&&index->free==1)
        {
          result = index;
          break;  
        }
        index = index -> next;
      }
    }break;

    case BEST_FIT:
    {
      if(base == NULL)    
      {
        return base;  
      }
      p_meta m_find = base;
      p_meta min = base;
      int min_size= 9999999;
      int found = 0;
      while(m_find != last && m_find != NULL)
      {
        if(m_find -> free == 1 && m_find ->size < min_size) 
        {
          if(m_find -> size + META_SIZE < size) continue;
          min = m_find;
          min_size = m_find -> size;
          found = 1;
        }
        m_find = m_find -> next;
      }
      if(found) result = min;
      else result = index;
    }
    break;

    case WORST_FIT:
    {
      if(base == NULL) return base;
      p_meta m_find = base;
      p_meta max = base;
      int max_size = 0;
      int found = 0;

      while(m_find != last && m_find != NULL)
      {
        if(m_find -> free == 1 && m_find -> size > max_size)
        {
          if(m_find -> size + META_SIZE < size) continue;
          
          max = m_find;
          max_size = m_find -> size;
          found = 1;
        }
        m_find = m_find -> next;  
      }
      if(found) result=max;
      else result=index;
    }
    break;
  }
  return result;
}

int getRealSize(size_t size)
{
  if(size % 4 != 0)
  {
    int div = size / 4;
    size = div * 4 + 4;
  }
  return size;
}


void *m_malloc(size_t size) {
  p_meta meta = find_meta(last,size);
  char *pos = NULL;
  
  if(meta == base)
  {
    pos = sbrk(META_SIZE + getRealSize(size));
    if(pos == -1) return NULL;
    meta = pos;
    meta -> next = NULL;

    if(base == NULL)
    {
      base = pos;
      meta -> prev = NULL;
      last = base;
    }else{
      meta -> prev = last;
      p_meta last_meta = last;
      last_meta -> next = meta;
      last = meta;
    }
  }else{
    p_meta new_meta = meta + getRealSize(size);
    new_meta -> prev = meta;
    new_meta -> next = meta -> next;
    meta -> next = new_meta;
    new_meta -> free = 1;
    new_meta -> size = meta -> size-getRealSize(size) - META_SIZE;
  }
  meta -> size = getRealSize(size);
  meta -> free = 0;
  return meta->data;
}

void m_free(void *ptr) {
  if(ptr == NULL) return;

  p_meta index = base;
  p_meta cur;
  
  while(index != last)
  {
    if(index ->data == ptr)
    {
      index -> free = 1;
      cur = index;
      p_meta search_free = cur;
    
      if(search_free -> next == NULL)
      {
        last = search_free -> prev;
      }

      while(search_free -> next != NULL && search_free -> next -> free ==1)
      {
        search_free -> size += getRealSize(search_free -> next -> size + META_SIZE);
        search_free -> next = search_free -> next -> next;
        search_free = search_free -> next;
      }
      search_free = cur;

      while(search_free -> prev != NULL && search_free -> prev -> free ==1)
      {
        search_free -> prev -> size += getRealSize(cur -> size) + META_SIZE;
        search_free -> prev -> next = cur -> next;
        search_free -> next -> prev = search_free -> prev;
        search_free = search_free -> prev; 
      }
      break;
    }
    index = index -> next;
  }
  p_meta last_meta = last;
  if(ptr == last_meta -> data)
  {
    int ret = brk(last_meta -> prev + last_meta -> prev -> size);
    last_meta -> prev -> next = NULL;
    last_meta -> free = 1;
    last = last_meta -> prev;
    
    //if(ret == -1) 
  }
}

void*m_realloc(void* ptr, size_t size){
  if(ptr == NULL) return;
  p_meta index = base;

  while(index != last)
  {
    if(index -> data == ptr)
    {
      if(getRealSize(size) == index -> size) return index;
      if(getRealSize(size) < index -> size)
      {
        int offset = getRealSize(size) - index -> size;
        index -> size += offset;
        
        if(offset < 0)
        {
          p_meta free_remain = m_malloc(offset*(-1) - META_SIZE);
          free_remain -> free = 1;
        }else{
          index -> next -> size -= offset;
        }
        break;
      }else {
        if(index -> size - size >= META_SIZE + sizeof(int))
        {
          int offset = index -> size - getRealSize(size);
          index -> size = getRealSize(size);
          index -> next -> size += offset;
        }
      break;
      } 
    }
    index = index -> next;
  }
  
  if(base == last)
  {
    if(index -> data == ptr)
    {
      if(index -> size -size >= META_SIZE + sizeof(int))
      {
        char str[index -> size];
        strcpy(str, index -> data);
        void *re_block = m_malloc(size);
        m_free(index -> data);
        strcpy(re_block, str);
      }
    }
  }
}
