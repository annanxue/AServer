#ifndef __MYMAP_H__
#define __MYMAP_H__

#include <sys/types.h>
#include <stack>
#include <set>
#include <assert.h>
#include "log.h"
#include "misc.h"
#include "autolock.h"

using namespace std;

#define INVALID_KEY	(u_long)0xFFFFFFFF


/*! utilfunc for MyMap*/

inline bool is_prime( u_long n )
{
	u_long i = 2, j = n;
	while( i < j ) 
	{
		if( n % i == 0 )
		{
			return false;
		}

		j = n / i++;
	}
	return true;
}


/*!< 只当hashsize为２的整数次幂的时候其产生的id才是均匀分布的，不是非常合理,
	但是，当为2的幂次的时候这个算法能够在u_long范围内，确保更加均匀的分布，
	所以还是采用这个方法，但一定要确保hashsize为2的幂次，这点可以在设置时进
	行检查。例如后面的set_size时。下面的is2power函数就是完成这个检查*/
inline u_long id_hash( u_long id, size_t HashSize )
{
	return (((id) + (id>>8) + (id>>16) + (id>>24)) & (HashSize-1));
}

/*!< 这个函数用来判断是否是2的幂,方法是判断该数字转换为2进制以后是否只有一个1*/
inline int is2power( int hash_id )
{
    int ret = 0;
    __asm__ __volatile__(
            "bsfl %1, %%eax\n\t"
            "bsrl %1, %%ecx\n\t"
            "movl $0, %0\n\t"
            "xorl %%ecx, %%eax\n\t"
            "jnz 1f\n\t"
            "movl $1, %0\n\t"
            "1:\n\t"
            :"=r"(ret)
            :"r"(hash_id)
            :"eax","ecx");

    return ret;
}

/*
* djb2
* this algorithm (k=33) was first reported by dan bernstein many years ago in 
* comp.lang.c. another version of this algorithm (now favored by bernstein)
* uses xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why
* it works better than many other constants, prime or not) has never been
* adequately explained. 
*/

inline u_long str_hash(const char *str, size_t size)
{
	if (NULL == str) return 0;

	u_long hash = 5381;
	
	int c;

	while ((c = *str++) && size--)
		hash = ((hash << 5) + hash) + c;	// hash * 33 + c 

	return hash;
}

/**********************************************************************************

 MyBucket

 *********************************************************************************/

template <class T> class MyBucket
{
public:
	MyBucket();
	virtual ~MyBucket();

public:
	void empty(void);
	bool is_empty(void);
	bool is_equal(u_long key);

public:
	u_long key_;
	T value_;
	MyBucket<T>* next_;			/*!< 单向链表的next指针*/
	MyBucket<T>* dprevious_;	/*!< 双向链表的previous指针*/
	MyBucket<T>* dnext_;		/*!< 双向链表的next指针*/
};


template <class T> MyBucket<T>::MyBucket()
{
	key_ = INVALID_KEY;
	next_ = NULL;
	dprevious_ = dnext_ = NULL;
}

template <class T> MyBucket<T>::~MyBucket()
{
}

template <class T> inline void MyBucket<T>::empty()
{
	key_ = INVALID_KEY;
}

template <class T> inline bool MyBucket<T>::is_empty()
{
	return (key_==INVALID_KEY);
}

template <class T> inline bool MyBucket<T>::is_equal(u_long key)
{
	return (key==key_);
}


/**********************************************************************************

 MyPool

 *********************************************************************************/

template <class T> class MyPool
{
public:
	MyPool();
	MyPool(size_t initial_size, size_t grow_size);
	virtual	~MyPool();

public:	
	void	push(T* mem);
	T*		pop(void);
	void	clear(void);
	void	set_size(size_t initial_size, size_t grow_size);
	size_t	get_initial_size(void);
	size_t	get_grow_size(void);

private:
	stack<T*>	stack_data_;	/*!< point of memory chunk asigned*/
	set<T*>		list_block_;	/*!< point of grow memory*/
	size_t		grow_size_;		/*!< grow size*/
	size_t		initial_size_;	/*!< initial size*/

private:
	void grow(size_t grow_size);

};

template <class T> MyPool<T>::MyPool()
{
	grow_size_ = initial_size_ = 0;
}

template <class T> MyPool<T>::MyPool( size_t initial_size, size_t grow_size )
{
	set_size(initial_size, grow_size);
}

template <class T> MyPool<T>::~MyPool()
{
	clear();
}

template <class T> inline void MyPool<T>::push(T* mem)
{
	assert( mem );
	assert( grow_size_>0 );
	
	stack_data_.push(mem);
}

template <class T> inline T* MyPool<T>::pop()
{
	assert( grow_size_>0 );
	if ( stack_data_.empty())
		grow(grow_size_);

	T* mem = stack_data_.top();
	stack_data_.pop();

	return mem;
}

template <class T> inline void MyPool<T>::clear(void)
{
	for ( typename set<T*>::iterator i = list_block_.begin(); i!=list_block_.end(); ++i)
	{
		delete[] *i;
	}
	list_block_.clear();

	while (!stack_data_.empty()) {
		stack_data_.pop();
	}

	initial_size_ = grow_size_ = 0;
}


template <class T> inline void MyPool<T>::set_size(size_t initial_size, size_t grow_size)
{
	assert(initial_size);
	assert(grow_size);

	clear();

	grow(initial_size);

	initial_size_	= initial_size;
	grow_size_		= grow_size;
}

template <class T> inline size_t MyPool<T>::get_initial_size(void)
{
	return initial_size_;
}

template <class T> inline size_t MyPool<T>::get_grow_size(void)
{
	return grow_size_;

}

template <class T> inline void MyPool<T>::grow(size_t grow_size)
{
	assert(grow_size>0);
//	LOG(2)( "mypool grows %s, size %d, num %d, total %d", typeid(T).name(), sizeof(T), grow_size, sizeof(T)*grow_size );
	T* block = new T[grow_size];
	list_block_.insert(block);
	for(size_t i=0; i<grow_size; ++i)
		stack_data_.push(&block[i]);
}


/**********************************************************************************

 MyMap( key by u_long)

 *********************************************************************************/

template <class T> class MyMap
{
public:
	MyMap();
	MyMap( size_t hash_size, size_t initial_size, size_t grow_size );
	virtual	~MyMap();

public:
	int		get_count( void ) const;
	void	set_size( size_t hash_size, size_t initial_size, size_t grow_size = 1 );
	bool	look_up( u_long key, T &value ) const;
    T*      find( u_long key ) const;

	void	set_at( u_long key, const T & value );
	void	set_at_safe( u_long key, const T & value );
	void	add_it_to_active_list( MyBucket<T>* bucket );
	void	remove_it_from_active_list( MyBucket<T>* bucket );
	void	remove_all( void );
	bool	remove_key( u_long key );	
	void	clear_active_list( void );
	MyBucket<T>*	get_first_active( void ) const;
	void	lock( void );
	void	unlock( void );
	
private:
	MyBucket<T>*				buckets_;
	MyPool< MyBucket<T> >		pool_;
	MyBucket<T>*				first_active_;
	size_t						hash_size_;
	u_long						total_;
public:
	Mutex						add_remove_lock_;
};

template <class T> MyMap<T>::MyMap(void)
{
	buckets_ = NULL;
	hash_size_ = 0;
	total_ = 0;
	first_active_ = NULL;
}

template <class T> MyMap<T>::MyMap(size_t hash_size, size_t initial_size, size_t grow_size)
{
	set_size(hash_size, initial_size, grow_size);	
}

template <class T> MyMap<T>::~MyMap()
{
	SAFE_DELETE_ARRAY(buckets_);
}

template <class T> inline int MyMap<T>::get_count(void) const
{
	return (int)total_;
}

template <class T> inline void MyMap<T>::set_size( size_t hash_size, size_t initial_size, size_t grow_size )
{
	assert( is2power(hash_size) );

	SAFE_DELETE_ARRAY(buckets_);
	first_active_ = NULL;

	buckets_ = new MyBucket<T>[hash_size_=hash_size];
	total_ = 0;
	pool_.set_size(initial_size, grow_size);
}

template <class T> inline bool MyMap<T>::look_up( u_long key, T &value ) const
{
	assert( hash_size_>0 );

	if (key==INVALID_KEY)
		return false;

	MyBucket<T>* bucket = &buckets_[id_hash(key, hash_size_)];
	while (bucket) 
	{
		if( bucket->is_equal(key))
		{
			value = bucket->value_;
			return true;
		}
		bucket = bucket->next_;
	}

	return false;
}

template <class T> inline T* MyMap<T>::find( u_long key ) const
{
	assert( hash_size_>0 );
	if (key==INVALID_KEY) return NULL;
	MyBucket<T>* bucket = &buckets_[id_hash(key, hash_size_)];
	while (bucket) {
		if( bucket->is_equal(key)) {
			return &(bucket->value_);
		}
		bucket = bucket->next_;
	}

	return NULL;
}




template <class T> inline void MyMap<T>::set_at( u_long key, const T & value )
{
	assert( hash_size_>0 );

	MyBucket<T> *bucket = NULL, *bucket_prev = NULL;
	total_++;
	for(bucket = &buckets_[(int)id_hash(key, hash_size_)]; bucket!=NULL; bucket = bucket->next_)
	{
		if (bucket->is_empty()) 
		{
			bucket->key_ = key;
			bucket->value_ = value;
			add_it_to_active_list(bucket);
			return;
		}
		bucket_prev = bucket;
	}

	bucket = bucket_prev->next_ = pool_.pop();
	bucket->key_ = key;
	bucket->value_ = value;
	add_it_to_active_list(bucket);
}


template <class T> inline void MyMap<T>::set_at_safe( u_long key, const T & value )
{
    lock();
    remove_key( key );
    set_at( key, value );
    unlock();
}

template <class T> inline void MyMap<T>::add_it_to_active_list( MyBucket<T>* bucket )
{
	assert(bucket);

	if (first_active_)
	{
		first_active_->dprevious_ = bucket;
	}

	bucket->dnext_ = first_active_;
	bucket->dprevious_ = NULL;
	first_active_ = bucket;
}

template <class T> inline void MyMap<T>::remove_it_from_active_list( MyBucket<T>* bucket )
{
	assert(bucket);

	if(bucket->dprevious_)
		bucket->dprevious_->dnext_ = bucket->dnext_;
	if(bucket->dnext_)
		bucket->dnext_->dprevious_ = bucket->dprevious_;
	if(first_active_ == bucket)
		first_active_ = bucket->dnext_;
}

template <class T> inline void MyMap<T>::remove_all( void )
{
	first_active_ = NULL;
	MyBucket<T> *bucket, *next_bucket; 
	for(size_t i=0; i<hash_size_; ++i)
	{
		bucket = &buckets_[i];
		if(!bucket->is_empty())
			bucket->empty();

		while (bucket->next_) 
		{
			next_bucket = bucket->next_;
			bucket->next_ = NULL;
			bucket = next_bucket;
			pool_.push(bucket);
		}
	}
	total_ = 0;
}

template <class T> inline bool MyMap<T>::remove_key( u_long key )
{
	assert(hash_size_>0);

	MyBucket<T> *bucket = &buckets_[(int)id_hash(key, hash_size_)], *prev_bucket;
	if (bucket->is_equal(key)) 
	{
		bucket->empty();
		remove_it_from_active_list(bucket);
		total_--;
		return true;
	}

	while (true)
	{
		prev_bucket = bucket;
		bucket = bucket->next_;
		if(!bucket)
			return false;

		if(bucket->is_equal(key))
		{
			prev_bucket->next_ = bucket->next_;
			bucket->next_ = NULL;
			remove_it_from_active_list(bucket);
			pool_.push(bucket);
			total_--;
			return true;
		}
	}

	return false;	
}

template <class T> inline void MyMap<T>::clear_active_list( void )
{
	first_active_	= NULL;
}

template <class T> inline MyBucket<T>* MyMap<T>::get_first_active( void ) const
{
	return first_active_;
}

template <class T> inline void MyMap<T>::lock( void )
{
	add_remove_lock_.Lock();
}

template <class T> inline void MyMap<T>::unlock( void )
{
	add_remove_lock_.Unlock();
}


/**********************************************************************************

 MyStrBucket (key by char*)

 *********************************************************************************/

template <class T> class MyStrBucket
{
public:
	MyStrBucket();
	virtual ~MyStrBucket();
	
public:
	void empty(void);
	bool is_equal(const char* key, size_t size);
	bool is_empty(void);
	
public:
	char key_[64];
	size_t size_;
	T value_;
	MyStrBucket<T>* next_;
	MyStrBucket<T>* dprevious_;
	MyStrBucket<T>* dnext_;	
};

template <class T> MyStrBucket<T>::MyStrBucket()
{
	size_ = 0;
	next_ = NULL;
	dprevious_ = dnext_ = NULL;
}
	
template <class T> MyStrBucket<T>::~MyStrBucket()
{
}

template <class T> inline void MyStrBucket<T>::empty(void)
{
	size_ = 0;	
}

template <class T> inline bool MyStrBucket<T>::is_equal(const char* key, size_t size)
{
	assert( key );
	
	if( size == size_ )
		return !strncmp( key, key_, size );
	return false;
}

template <class T> inline bool MyStrBucket<T>::is_empty(void)
{
	return( !size_ );
}


/**********************************************************************************

 MyStrMap

 *********************************************************************************/

template <class T> class MyStrMap
{
public:
	MyStrMap();
	MyStrMap( size_t hash_size, size_t initial_size, size_t grow_size );
	virtual ~MyStrMap();
	
public:
	int get_count(void) const;
	void set_size( size_t hash_size, size_t initial_size, size_t grow_size = 1 );
	bool look_up(const char* key, T& value) const;
	void set_at(const char* key, const T &value);
	void remove_all(void);
	bool remove_key(const char* key);
	void add_it_to_active_list( MyStrBucket<T>* bucket);
	void remove_it_from_active_list( MyStrBucket<T>* bucket);
	MyStrBucket<T>* get_first_active(void) const;
	void clear_active_list(void);
	void lock(void);
	void unlock(void);
	
	MyStrBucket<T>* buckets_;
	u_long total_;
	u_long hash_size_;
private:
	MyPool< MyStrBucket<T> > pool_;
	MyStrBucket<T>* first_active_;
	Mutex	add_remove_lock_;//from public to private	
};

template <class T> MyStrMap<T>::MyStrMap()
{
	buckets_ = NULL;
	hash_size_ = 0;
	total_ = 0;
	first_active_ = NULL;
}

template <class T> MyStrMap<T>::MyStrMap( size_t hash_size, size_t initial_size, size_t grow_size )
{
	set_size( hash_size, initial_size, grow_size );
}
	
template <class T> MyStrMap<T>::~MyStrMap()
{
	SAFE_DELETE_ARRAY( buckets_ );
}
	
template <class T> int MyStrMap<T>::get_count(void) const
{
	return (int)total_;
}

template <class T> void MyStrMap<T>::set_size( size_t hash_size, size_t initial_size, size_t grow_size )
{
	assert( is2power(hash_size) );

	SAFE_DELETE_ARRAY( buckets_ );
	first_active_ = NULL;

	buckets_ = new MyStrBucket<T>[hash_size_=hash_size];
	total_ = 0;
	pool_.set_size(initial_size, grow_size);
}
	
template <class T> bool MyStrMap<T>::look_up(const char* key, T& value) const
{
	assert(hash_size_ > 0);
	assert( key );

	if (key == NULL)
		return false;	
	
	size_t size = strlen(key);
	if( size == 0 )
		return false;
	
	int hash_pos=0;
	hash_pos = (int)id_hash( str_hash(key, size), hash_size_);
	
	MyStrBucket<T>* bucket = &buckets_[(int)id_hash( str_hash(key, size), hash_size_)];
	while( bucket )
	{
		if( bucket->is_equal( key, size ))
		{
			value = bucket->value_;

			return true;
		}

		bucket = bucket->next_;
	}

	return false;
}
	
template <class T> void MyStrMap<T>::set_at(const char* key, const T &value)
{	
	assert( hash_size_ > 0 );
	assert( key );
		
	MyStrBucket<T> *bucket=NULL, *prev_bucket=NULL;
	size_t size = strlen( key );
	total_++;
	
	for(bucket = &buckets_[(int)id_hash( str_hash( key, size ), hash_size_ )]; bucket != NULL; bucket = bucket->next_)
	{
		if( bucket->is_empty())
		{
			strlcpy( bucket->key_, key, 64 );
			bucket->size_ = size;
			bucket->value_ = value;
			add_it_to_active_list( bucket );
			return;
		}
		prev_bucket = bucket;
	}
	
	bucket = prev_bucket->next_ = pool_.pop();
	strlcpy( bucket->key_, key, 64 );
	bucket->size_ = size;
	bucket->value_ = value;
	add_it_to_active_list( bucket );
}
	
template <class T> void MyStrMap<T>::remove_all(void)
{
	first_active_ = NULL;
	MyStrBucket<T> *bucket, *next_bucket;
	
	for(u_long i=0; i<hash_size_; ++i)
	{
		bucket = &buckets_[i];
		if(!bucket->is_empty())
			bucket->empty();
			
		while( bucket->next_)
		{
			next_bucket = bucket->next_;
			bucket->next_ = NULL;
			bucket = next_bucket;
			pool_.push( bucket );
		}
	}
	total_ = 0;
}
	
template <class T> bool MyStrMap<T>::remove_key(const char* key)
{	
	assert( hash_size_ > 0);
	assert( key );
		
	size_t size = strlen( key );
	MyStrBucket<T> *bucket = &buckets_[(int)id_hash( str_hash( key, size ), hash_size_)], *prev_bucket;
	if( bucket->is_equal( key, size ))
	{
		bucket->empty();
		remove_it_from_active_list( bucket );
		total_--;
		return true;
	}
	while( true )
	{
		prev_bucket = bucket;
		bucket = bucket->next_;
		if(!bucket)
			return false;
		if(bucket->is_equal( key, size ))
		{
			prev_bucket->next_ = bucket->next_;
			bucket->next_ = NULL;
			remove_it_from_active_list( bucket );
			pool_.push( bucket );
			total_--;
			return true;
		}		
	}
	return false;
}
	
template <class T> void MyStrMap<T>::add_it_to_active_list( MyStrBucket<T>* bucket)
{
	assert( bucket );
	
	if (first_active_)
	{
		first_active_->dprevious_ = bucket;
	}
	bucket->dnext_ = first_active_;
	bucket->dprevious_ = NULL;
	first_active_ = bucket;
}
	
template <class T> void MyStrMap<T>::remove_it_from_active_list( MyStrBucket<T>* bucket)
{
	assert( bucket );
	
	if( bucket->dprevious_ )
		bucket->dprevious_->dnext_ = bucket->dnext_;
	if( bucket->dnext_ )
		bucket->dnext_->dprevious_ = bucket->dprevious_;
	if( first_active_ == bucket )
		first_active_ = bucket->dnext_;
}
	
template <class T> MyStrBucket<T>* MyStrMap<T>::get_first_active(void) const
{
	return first_active_;
}
	
template <class T> void MyStrMap<T>::clear_active_list(void)
{
	first_active_ = NULL;
}
	
template <class T> void MyStrMap<T>::lock(void)
{
	add_remove_lock_.Lock();
}
	
template <class T> void MyStrMap<T>::unlock(void)
{
	add_remove_lock_.Unlock();
}


#endif

