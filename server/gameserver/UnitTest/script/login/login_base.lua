module( "login_mng",  package.seeall )

-- ------------------------------------
-- Counter ADT
-- ------------------------------------
counter_t = counter_t or {}
counter_t.__index = counter_t

function counter_t.new()
    local counter = { count_ = 0 }
    setmetatable( counter, counter_t )
    return counter
end

function counter_t.increase_by_one( self, _key )
    if not self[ _key ] then
        self.count_ = self.count_ + 1
        self[ _key ] = true
    end
end

function counter_t.decrease_by_one( self, _key )
    if self[ _key ] then
        self.count_ = self.count_ - 1
        self[ _key ] = nil
    end
end

function counter_t.count( self )
    return self.count_
end

function counter_t.has_key( self, _key )
    return self[ _key ]
end

-- ------------------------------------
-- Login Queue ADT 
-- ------------------------------------

login_queue_t = login_queue_t or {}
login_queue_t.__index = login_queue_t

function login_queue_t.new( _size )
    local queue = { tail_ = -1, head_ = 0, size_ = _size, empty_ = 0, map_ = {} } 
    setmetatable( queue, login_queue_t )
    return queue
end

function login_queue_t.enqueue( self, _elem )
    if self:is_full() then
        c_err( "[login_base](login_queue_t.enqueue) try to enqueue an elem into a full queue" )
        return false
    end

    if self.map_[ _elem.key_ ] then
        c_err( "[login_base](login_queue_t.enqueue) try to enqueue a reduplicative key elem into the queue" )
        return false
    end
    
    local tail = self.tail_ + 1
    self.tail_ = tail
    self[ tail ] = _elem
    self.map_[ _elem.key_ ] = tail
    return tail - self.head_ + 1 - self.empty_
end

function login_queue_t.dequeue( self )
    if self:is_empty() then
        c_err( "[login_base](login_queue_t.dequeue) try to dequeue an elem from an empty queue" )
        return false
    end

    for head = self.head_, self.tail_ do
        local elem = self[ head ] 
        if elem then
            self[ head ] = nil
            self.map_[ elem.key_ ] = nil
            self.head_ = head + 1
            return elem
        else
            self.empty_ = self.empty_ - 1
        end
    end
    
    return false
end

function login_queue_t.remove( self, _key )
    if self:is_empty() then
        c_err( "[login_base](login_queue_t.remove) try to remove an elem from an empty queue" )
        return false
    end

    local index = self.map_[ _key ] 
    if not index then
        c_err( "[login_base](login_queue_t.remove) can not find the elem with key: %s in the queue's map", tostring( _key ) )
        return false
    end

    local elem = self[ index ] 
    if not elem then
        c_err( "[login_base](login_queue_t.remove) can not find the elem with key: %s at the expected index: %d in the queue", tostring( _key ), index )
        return false
    end
    
    self.map_[ _key ] = nil
    self[ index ] = nil
    self.empty_ = self.empty_ + 1
    return elem
end

function login_queue_t.replace_key( self, _new_key, _old_key )
    if _new_key == _old_key then 
        return true 
    end
    
    if self:is_in_queue( _new_key ) then
        c_err( "[login_base](login_queue_t.replace_key) the elem with the new key: %s is already in the queue", tostring( _new_key ) )
        return false
    end

    if not self:is_in_queue( _old_key ) then
        c_err( "[login_base](login_queue_t.replace_key) can not find the elem with the old key: %s", tostring( _old_key ) )
        return false
    end

    local index = self.map_[ _old_key ] 
    local elem = self[ index ]

    elem.key_ = _new_key 
    self.map_[ _new_key ] = index
    self.map_[ _old_key ] = nil
    return true
end

function login_queue_t.get( self, _key )
    if self:is_in_queue( _key ) then
        local index = self.map_[ _key ]
        return self[ index ] 
    end
end

function login_queue_t.is_in_queue( self, _key )
    if self:is_empty() then
        return false
    end

    local index = self.map_[ _key ]
    if not index then 
        return false
    end

    local elem = self[ index ] 
    if elem then
        return true
    else 
        return false
    end
end

function login_queue_t.count( self )
    if self.tail_ < self.head_ then 
        return 0 
    end
    return self.tail_ - self.head_ + 1 - self.empty_
end

function login_queue_t.is_empty( self )
    return self.tail_ - self.head_ + 1 == self.empty_
end

function login_queue_t.is_full( self )
    return self.tail_ - self.head_ + 1 == self.size_ + self.empty_ 
end

function login_queue_t.map( self, _func )
    if _func and type( _func ) == "function" then
        local empty = 0 
        for i = self.head_, self.tail_ do
            local elem = self[ i ] 
            if elem then
                _func( elem, i - self.head_ + 1 - empty )
            else
                empty = empty + 1
            end
        end
    end
end
