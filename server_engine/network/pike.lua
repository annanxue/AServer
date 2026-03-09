--
--	$Id: pike.lua 15870 2006-10-31 14:12:35Z jamesdeng $
--

--
-- ff_ctx.buffer is dword type, and result is table
--

ff_ctx_send = {}


function linearity_send( key )
	local result_key = 0
	result_key = (((( key >> 31 ) # ( key >> 6 ) # ( key >> 4 ) # ( key >> 2 ) # ( key >> 1 ) # key ) & 1 ) << 31) | ( key >> 1 )
	return result_key
end

function addikey_next_send( key )	
	local tmpresult, i1, i2
	local ff_ctx = ff_ctx_send
	local addikey = ff_ctx.addikey[key]
	local addikey_buffer = addikey.buffer
	
	local addikey_value_index = (addikey.index + 1) & 63
	addikey.index = addikey_value_index
	local addikey_value_index_or_64 = addikey_value_index | 64

	local addikey_buffer_value_i1, addikey_buffer_value_i2

	i1 = ( addikey_value_index_or_64 - addikey.dis1 ) & 63
	i2 = ( addikey_value_index_or_64 - addikey.dis2 ) & 63

	addikey_buffer_value_i1 = addikey_buffer[i1]
	addikey_buffer_value_i2 = addikey_buffer[i2]
	
	tmpresult = addikey_buffer_value_i1 + addikey_buffer_value_i2
	if (tmpresult>4294967295) then  --overflow on this
		tmpresult = tmpresult - 4294967296
	end
	addikey_buffer[addikey_value_index] = tmpresult

	if ( tmpresult < addikey_buffer_value_i1 ) or  ( tmpresult < addikey_buffer_value_i2) then
		addikey.carry = 1
	else
		addikey.carry = 0
	end
end

function ctx_generate_send()
	local ff_ctx = ff_ctx_send
	local carry, value
	local addikey_0, addikey_1, addikey_2 = ff_ctx.addikey[0], ff_ctx.addikey[1],ff_ctx.addikey[2]
	local addikey_0_buffer, addikey_1_buffer, addikey_2_buffer = addikey_0.buffer, addikey_1.buffer, addikey_2.buffer

	for i=0, 1023, 1 do
		carry = addikey_0.carry + addikey_1.carry + addikey_2.carry

		if ( carry==0 ) or (carry==3) then
				addikey_next_send( 0 )
				addikey_next_send( 1 )
				addikey_next_send( 2 )
		else
				if(carry==2) then
						if (addikey_0.carry == 1) then
							addikey_next_send( 0 )
						end
						if (addikey_1.carry == 1) then
							addikey_next_send( 1 )
						end
						if (addikey_2.carry == 1) then
							addikey_next_send( 2 )
						end
				else
						if (addikey_0.carry == 0) then
							addikey_next_send( 0 )
						end
						if (addikey_1.carry == 0) then
							addikey_next_send( 1 )
						end
						if (addikey_2.carry == 0) then
							addikey_next_send( 2 )
						end
				end
		end
	
		value = addikey_0_buffer[addikey_0.index] # addikey_1_buffer[addikey_1.index]
		value = value # addikey_2_buffer[addikey_2.index]
		
		--buffer as dword
		ff_ctx.buffer[i] = value
	end

	ff_ctx.index = 0
end

function ctx_init_send( seed )
	--create ff_ctx
	local tmp
	local sd_temp = seed # 84048153


	ff_ctx_send = {
		sd = sd_temp,
		index = 0,
		addikey = {
			[0] = { sd = 0, dis1 = 0, dis2 = 0, index = 0, carry = 0, buffer={}  },
			[1] = { sd = 0, dis1 = 0, dis2 = 0, index = 0, carry = 0, buffer={}  },
			[2] = { sd = 0, dis1 = 0, dis2 = 0, index = 0, carry = 0, buffer={}  }
		},
		buffer = {}
	}
	
	for i=0, 1023, 1 do  --change for dword
		ff_ctx_send.buffer[i] = 0
	end

	for i=0, 63, 1 do
		ff_ctx_send.addikey[0].buffer[i] = 0
		ff_ctx_send.addikey[0].buffer[i] = 0
		ff_ctx_send.addikey[0].buffer[i] = 0
	end

	--init ff_ctx with see
	ff_ctx_send.addikey[0].sd = ff_ctx_send.sd
	ff_ctx_send.addikey[0].sd = linearity_send( ff_ctx_send.addikey[0].sd )
	ff_ctx_send.addikey[0].dis1 = 55
	ff_ctx_send.addikey[0].dis2 = 24

	-- 1431655765=0x55555555, 2863311530=0xAAAAAAAA 
	ff_ctx_send.addikey[1].sd = ((ff_ctx_send.sd & 2863311530) >> 1 ) | (( ff_ctx_send.sd & 1431655765 ) << 1 )
	ff_ctx_send.addikey[1].sd = linearity_send( ff_ctx_send.addikey[1].sd )
	ff_ctx_send.addikey[1].dis1 = 57
	ff_ctx_send.addikey[1].dis2 = 7
	
	-- 4042322160=0xF0F0F0F0, 252645135=0x0F0F0F0F
	ff_ctx_send.addikey[2].sd = ~(((ff_ctx_send.sd & 4042322160) >> 4) | (( ff_ctx_send.sd & 252645135 ) << 4 ))
	ff_ctx_send.addikey[2].sd = linearity_send( ff_ctx_send.addikey[2].sd )
	ff_ctx_send.addikey[2].dis1 = 58
	ff_ctx_send.addikey[2].dis2 = 19

	for i=0, 2, 1 do
		tmp = ff_ctx_send.addikey[i].sd
		for j=0, 63, 1 do
			for k=0, 31, 1 do
				tmp = linearity_send( tmp )
			end
			ff_ctx_send.addikey[i].buffer[j] = tmp
		end
		ff_ctx_send.addikey[i].carry = 0
		ff_ctx_send.addikey[i].index = 63
	end	
	
	ff_ctx_send.index = 4096

end


function ctx_encode_send( len, buf )
	local ff_ctx = ff_ctx_send
	local ff_ctx_buffer = ff_ctx.buffer
	local result_table = { [1]={}, [2]={}, [3]={} }
	local byte_value, byte_value_2, dword_value, ptr, remnant
	local k, div_value, mod_value
	local buf_pos, p1_pos, p2_pos, p3_pos = 1, 1, 1, 1

	while (len>0) do
		remnant = 4096 - ff_ctx.index

		if ( remnant < 1 ) then
			ctx_generate_send()
		else
			if ( remnant > len ) then
				remnant = len
			end
			len = len - remnant

			ptr = ff_ctx.index

			k = 1
			mod_value = math.mod( ptr, 4 )
			div_value = (ptr - mod_value)/4
			if (ptr~=0) and (mod_value~=0)  then
				for i=4-mod_value, 1, -1 do
					byte_value = string.byte( buf, buf_pos )
					byte_value = byte_value # (( (ff_ctx.buffer[div_value] & ( 4278190080 >> ( (i-1)*8 ) ) ) << ( (i-1)*8 )) >> 24 ) 
					
					result_table[1][p1_pos] = byte_value
					k = k + 1
					buf_pos = buf_pos + 1
					p1_pos = p1_pos + 1
				end
				--if (k>1) then
					div_value = div_value + 1
				--end
			end

			for j=k, remnant-3, 4 do
				dword_value = string.dword( buf, buf_pos )
				dword_value = dword_value # ff_ctx.buffer[div_value]
				result_table[2][p2_pos] = dword_value
				div_value = div_value + 1
				k = k + 4
				buf_pos = buf_pos + 4
				p2_pos = p2_pos + 1
			end

			for m=k, remnant, 1 do
				byte_value_2 = string.byte( buf, buf_pos )
				byte_value_2 = byte_value_2 # ( (ff_ctx.buffer[div_value] & ( 255 << ( (m-k)*8 ) )) >> ( (m-k)*8 ) )
				result_table[3][p3_pos] = byte_value_2
				buf_pos = buf_pos + 1
				p3_pos = p3_pos + 1
			end

			ff_ctx.index = ff_ctx.index + remnant
		end
	end

	return result_table

--[[
tmp_table = { [1]={}, [2]={}, [3]={} }
tmp_table[1][1] = 99
tmp_table[1][2] = 98
tmp_table[1][3] = 97
tmp_table[1][4] = 96
tmp_table[2][1] = 95
tmp_table[3][2] = 94
tmp_table[3][1] = 93
tmp_table[3][2] = 92
tmp_table[3][3] = 91

	return tmp_table
--	return 12]]

end



--
-- ff_ctx_recv for recv
--

ff_ctx_recv = {}


function linearity_recv( key )
	local result_key = 0 
	result_key = (((( key >> 31 ) # ( key >> 6 ) # ( key >> 4 ) # ( key >> 2 ) # ( key >> 1 ) # key ) & 1 ) << 31) | ( key >> 1 )
	return result_key
end

function addikey_next_recv( key )	
	local tmpresult, i1, i2
	local ff_ctx = ff_ctx_recv
	local addikey = ff_ctx.addikey[key]
	local addikey_buffer = addikey.buffer
	
	local addikey_value_index = (addikey.index + 1) & 63
	addikey.index = addikey_value_index
	local addikey_value_index_or_64 = addikey_value_index | 64

	local addikey_buffer_value_i1, addikey_buffer_value_i2

	i1 = ( addikey_value_index_or_64 - addikey.dis1 ) & 63
	i2 = ( addikey_value_index_or_64 - addikey.dis2 ) & 63

	addikey_buffer_value_i1 = addikey_buffer[i1]
	addikey_buffer_value_i2 = addikey_buffer[i2]
	
	tmpresult = addikey_buffer_value_i1 + addikey_buffer_value_i2
	if (tmpresult>4294967295) then  --overflow on this
		tmpresult = tmpresult - 4294967296
	end
	addikey_buffer[addikey_value_index] = tmpresult

	if ( tmpresult < addikey_buffer_value_i1 ) or  ( tmpresult < addikey_buffer_value_i2) then
		addikey.carry = 1
	else
		addikey.carry = 0
	end
end

function ctx_generate_recv()
	local ff_ctx = ff_ctx_recv
	local carry, value
	local addikey_0, addikey_1, addikey_2 = ff_ctx.addikey[0], ff_ctx.addikey[1],ff_ctx.addikey[2]
	local addikey_0_buffer, addikey_1_buffer, addikey_2_buffer = addikey_0.buffer, addikey_1.buffer, addikey_2.buffer

	for i=0, 1023, 1 do
		carry = addikey_0.carry + addikey_1.carry + addikey_2.carry

		if ( carry==0 ) or (carry==3) then
				addikey_next_recv( 0 )
				addikey_next_recv( 1 )
				addikey_next_recv( 2 )
		else
				if(carry==2) then
						if (addikey_0.carry == 1) then
							addikey_next_recv( 0 )
						end
						if (addikey_1.carry == 1) then
							addikey_next_recv( 1 )
						end
						if (addikey_2.carry == 1) then
							addikey_next_recv( 2 )
						end
				else
						if (addikey_0.carry == 0) then
							addikey_next_recv( 0 )
						end
						if (addikey_1.carry == 0) then
							addikey_next_recv( 1 )
						end
						if (addikey_2.carry == 0) then
							addikey_next_recv( 2 )
						end
				end
		end
	
		value = addikey_0_buffer[addikey_0.index] # addikey_1_buffer[addikey_1.index]
		value = value # addikey_2_buffer[addikey_2.index]
		
		--buffer as dword
		ff_ctx.buffer[i] = value
	end

	ff_ctx.index = 0
end

function ctx_init_recv( seed )

	--create ff_ctx
	local tmp
	local sd_temp = seed # 84048153

	ff_ctx_recv = {
		sd = sd_temp,
		index = 0,
		addikey = {
			[0] = { sd = 0, dis1 = 0, dis2 = 0, index = 0, carry = 0, buffer={}  },
			[1] = { sd = 0, dis1 = 0, dis2 = 0, index = 0, carry = 0, buffer={}  },
			[2] = { sd = 0, dis1 = 0, dis2 = 0, index = 0, carry = 0, buffer={}  }
		},
		buffer = {}
	}
	
	for i=0, 1023, 1 do  --change for dword
		ff_ctx_recv.buffer[i] = 0
	end

	for i=0, 63, 1 do
		ff_ctx_recv.addikey[0].buffer[i] = 0
		ff_ctx_recv.addikey[0].buffer[i] = 0
		ff_ctx_recv.addikey[0].buffer[i] = 0
	end

	--init ff_ctx with see
	ff_ctx_recv.addikey[0].sd = ff_ctx_recv.sd
	ff_ctx_recv.addikey[0].sd = linearity_recv( ff_ctx_recv.addikey[0].sd )
	ff_ctx_recv.addikey[0].dis1 = 55
	ff_ctx_recv.addikey[0].dis2 = 24

	-- 1431655765=0x55555555, 2863311530=0xAAAAAAAA 
	ff_ctx_recv.addikey[1].sd = ((ff_ctx_recv.sd & 2863311530) >> 1 ) | (( ff_ctx_recv.sd & 1431655765 ) << 1 )
	ff_ctx_recv.addikey[1].sd = linearity_recv( ff_ctx_recv.addikey[1].sd )
	ff_ctx_recv.addikey[1].dis1 = 57
	ff_ctx_recv.addikey[1].dis2 = 7
	
	-- 4042322160=0xF0F0F0F0, 252645135=0x0F0F0F0F
	ff_ctx_recv.addikey[2].sd = ~(((ff_ctx_recv.sd & 4042322160) >> 4) | (( ff_ctx_recv.sd & 252645135 ) << 4 ))
	ff_ctx_recv.addikey[2].sd = linearity_recv( ff_ctx_recv.addikey[2].sd )
	ff_ctx_recv.addikey[2].dis1 = 58
	ff_ctx_recv.addikey[2].dis2 = 19

	for i=0, 2, 1 do
		tmp = ff_ctx_recv.addikey[i].sd
		for j=0, 63, 1 do
			for k=0, 31, 1 do
				tmp = linearity_recv( tmp )
			end
			ff_ctx_recv.addikey[i].buffer[j] = tmp
		end
		ff_ctx_recv.addikey[i].carry = 0
		ff_ctx_recv.addikey[i].index = 63
	end	
	
	ff_ctx_recv.index = 4096
end


function ctx_encode_recv( len, buf )
	local ff_ctx = ff_ctx_recv
	local ff_ctx_buffer = ff_ctx.buffer
	local result_table = { [1]={}, [2]={}, [3]={} }
	local byte_value, byte_value_2, dword_value, ptr, remnant
	local k, div_value, mod_value
	local buf_pos, p1_pos, p2_pos, p3_pos = 1, 1, 1, 1

	while (len>0) do
		remnant = 4096 - ff_ctx.index

		if ( remnant < 1 ) then
			ctx_generate_recv()
		else
			if ( remnant > len ) then
				remnant = len
			end
			len = len - remnant

			ptr = ff_ctx.index

			k = 1
			mod_value = math.mod( ptr, 4 )
			div_value = (ptr - mod_value)/4
			if (ptr~=0) then
				for i=4-mod_value, 1, -1 do
					byte_value = string.byte( buf, buf_pos )
					byte_value = byte_value # (( (ff_ctx.buffer[div_value] & ( 4278190080 >> ( (i-1)*8 ) ) ) << ( (i-1)*8 )) >> 24 ) 
					result_table[1][p1_pos] = byte_value
					k = k + 1
					buf_pos = buf_pos + 1
					p1_pos = p1_pos + 1
				end
				if (k>1) then
					div_value = div_value + 1
				end
			end

			for j=k, remnant-3, 4 do
				dword_value = string.dword( buf, buf_pos )
				dword_value = dword_value # ff_ctx.buffer[div_value]
				result_table[2][p2_pos] = dword_value
				div_value = div_value + 1
				k = k + 4
				buf_pos = buf_pos + 4
				p2_pos = p2_pos + 1
			end

			for m=k, remnant, 1 do
				byte_value_2 = string.byte( buf, buf_pos )
				byte_value_2 = byte_value_2 # ( (ff_ctx.buffer[div_value] & ( 255 << ( (m-k)*8 ) )) >> ( (m-k)*8 ) )
				result_table[3][p3_pos] = byte_value_2
				buf_pos = buf_pos + 1
				p3_pos = p3_pos + 1
			end

			ff_ctx.index = ff_ctx.index + remnant
		end
	end

	return result_table
end




