module( "test_equip", package.seeall )
local EQUIPS =EQUIPS 
local ITEMS = ITEMS
local EQUIP_ATTRS = EQUIP_ATTRS
local RANDOM_ITEM_LIBS = RANDOM_ITEM_LIBS
local ORANGE_EQUIP_ASCEND_MATERIAL = ORANGE_EQUIP_ASCEND_MATERIAL
local ipairs, random, tremove, sgsub, sformat = ipairs, math.random, table.remove, string.gsub, string.format
local table_empty = utils.table.is_empty

NO_CHECK_EQUIP =
{
    [160431] = 1,
    [260431] = 1,
    [360431] = 1,
    [120431] = 1,
    [220431] = 1,
    [320431] = 1,
    [170431] = 1,
    [160121] = 1,
    [260121] = 1,
    [360121] = 1,
    [120422] = 1,
    [220422] = 1,
    [320422] = 1,
    [160422] = 1,
    [260422] = 1,
    [360422] = 1,
    [160523] = 1,
    [360523] = 1,
    [260523] = 1,
    [360533] = 1,
    [160533] = 1,
    [260533] = 1,
}

NO_CHECK_EQUIP_BIND =
{
    [160523] = 1,
    [160533] = 1,
    [260523] = 1,
    [260533] = 1,
    [360523] = 1,
    [360533] = 1,
    [160756] = 1,
    [260756] = 1,
    [360756] = 1,
    [120553] = 1,
    [220553] = 1,
    [320553] = 1,
    [180753] = 1,
    [280753] = 1,
    [160956] = 1,
    [260956] = 1,
    [360956] = 1,
    [160553] = 1,
    [260553] = 1,
    [360553] = 1,
}

NO_CHECK_FIRST_BUY_EQUIPS =
{
    [160553] = 1,
    [260553] = 1,
    [360553] = 1,
    [160533] = 1,
    [160523] = 1,
    [260533] = 1,
    [260523] = 1,
    [360533] = 1,
    [360523] = 1,
}
---装备部位检查[1,9]以及职业限制饰品无职业限制 
local function test_part( )
    local check_result = true

    for equip_id,equip_info in pairs( EQUIPS ) do
        local part = equip_info.EquipPart

        if ITEMS[equip_id] then
            if ( not part ) or ( type(part) ~= "number" ) or ( part > 9 ) or ( part < 1 ) then
                check_result = false
                c_err(sformat("Equip_id:%d EquipPart is:%d is not 1~9!",equip_id,part))
            end

            if part and ITEMS[equip_id].JobLimit then
                if ( part >= 1 and part <= 6 ) and part ~= 4  and ITEMS[equip_id].JobLimit == 0 then
                    check_result = false
                    c_err(sformat("Equip_id:%d EquipPart:%d but JobLimit is 0!",equip_id, part))
                end

                --add by kent
                --if ( part >= 7 )  and ITEMS[equip_id].JobLimit ~= 0 then
                    --check_result = false
                    --c_err(sformat("Equip_id:%d EquipPart:%d but JobLimit is %d!",equip_id, part, ITEMS[equip_id].JobLimit))
                --end

                --if part == 4 and ITEMS[equip_id].JobLimit ~= 0 then
                    --check_result = false
                    --c_err(sformat("Equip_id: %d EquipPart is 4 but JobLimit is != 0",equip_id))
                --end
                --
            end
        end
    end
    return check_result
end

---装备等级品质检查 [1,100]and[1,5]
local function test_equip_level( )
    local check_result = true

    for equip_id,equip_info in pairs( EQUIPS ) do

        if ITEMS[equip_id] and not NO_CHECK_FIRST_BUY_EQUIPS[equip_id] then
            local uselevel = ITEMS[equip_id].UseLevel
            --add by kent
            --if  not uselevel  or ( type(uselevel) ~= "number" ) or  uselevel < 1  or  uselevel >100  then
            if  not uselevel  or ( type(uselevel) ~= "number" ) or  uselevel < 1  or  uselevel >120  then
                check_result = false
                c_err(sformat("Equip_id:%d UseLevel is: %d not 1~120!",equip_id,uselevel))
            end
            if ITEMS[equip_id].Quality < 1 or ITEMS[equip_id].Quality >5 then
                check_result = false
                c_err(sformat("Equip_id:%d Quality: %d is wrong",equip_id,ITEMS[equip_id].Quality))
            end
            -- 策划现在要求低于30级装备也能配成橙装   do by abel 2018-5-24
            -- if uselevel <= 30 and ITEMS[equip_id].Quality == 5 then
                -- check_result = false
                -- c_err(sformat("Equip_id:%d UseLevel is: %d Quality: %d can't =5",equip_id,uselevel,ITEMS[equip_id].Quality))
            -- end
        end
    end
    return check_result
end

----装备id和装备最大堆叠数=1检查
local function test_pack_max( )
    local check_result = true

    for equip_id,equip_info in pairs( EQUIPS ) do
        if not ITEMS[equip_id] then
            check_result = false
            c_err(sformat("Equip_id:%d can find in ITEMS!",equip_id))
        end

        if ITEMS[equip_id] and ITEMS[equip_id].PackMax ~= 1 then
            check_result = false
            c_err(sformat("Equip_id: %d PackMax: %d  is not 1!",equip_id,ITEMS[equip_id].PackMax))
        end
    end   
    return check_result
end

---装备属性条目最大检查[6,8]
local function test_attr_num( )
    local check_result = true

    for equip_id,equip_info in pairs( EQUIPS ) do
       local attr_num = equip_info.AttrNumMax 
       if table_empty( attr_num ) then
          check_result = false 
          c_err(sformat("Equip_id: %d AttrNumMax is empty !",equip_id))
       end
       if attr_num and #attr_num >= 3 then
           if attr_num[1][1] < 6 or attr_num[3][1] > 8 then
               check_result = false
               c_err(sformat("Equip_id:%d AttrNumMin: %d AttrNumMax: %d is not between[6,8]!", equip_id,attr_num[1][1],attr_num[3][1]))
           end
       end
    end
    return check_result 
end

--装备固定属性条目数量和正确性检查
local function test_fix_attr( )
    local check_result = true

    for equip_id,equip_info in pairs( EQUIPS )do
        local fix_attr = equip_info.FixAttr
        if table_empty( fix_attr) then
            check_result = false
            c_err(sformat("Equip_id: %d FixAttr is empty !",equip_id))
        end
        if fix_attr and #fix_attr > 2 then
            check_result = false
            c_err(sformat("Equip_id: %d FixAttr more then 2!",equip_id))
        end
        if fix_attr and not check_attr( fix_attr ) then
            check_result = false 
            c_err(sformat("Equip_id: %d FixAttr not in EQUIP_ATTRS!",equip_id))
        end
    end
    return check_result 
end

--检查属性
function check_attr( attr_ )

    for _,attr_index in pairs ( attr_ ) do 
        if not EQUIP_ATTRS[attr_index] then
           c_err(sformat("Attr_id is: %d",attr_index))
           return false
        end
     end
     return true
end

--取到属性条目最多数
local function get_max_attr( attr_rule_)
    local max_num = 0
 
    if attr_rule_ then
        for i,v in pairs (attr_rule_) do
            if v[2] > 0 and v[1] > max_num  then
                max_num = v[1]
            end
        end
    end
    return max_num
end

---装备随机属性1条目库属性数量检查
local function test_random_attr1( )
     local check_result = true

     for equip_id,equip_info in pairs ( EQUIPS ) do
         local random1 = equip_info.RandomAttr1
         local rule1 = equip_info.RandomAttrRule1
         local attr_max = 0
         if rule1 then
            attr_max = get_max_attr( rule1 )
         end
         if attr_max > 0 and  not random1 then
            check_result = false
            c_err(sformat("equip_id is: %d RandomAttr1 is nil,but RandomAttr1_max is: %d",equip_id,attr_max))
         end
         if random1 and not check_attr( random1 )then
            check_result = false
            c_err(sformat("equip_id is: %d RandomAttr1 is not EQUIP_ATTRS !",equip_id))
         end
         if random1 and #random1 < attr_max then
            check_result = false
            c_err(sformat("equip_id is: %d len(RandomAttr1):%d < rule_max_num: %d",equip_id,#random1,attr_max))
         end
     end
     return check_result 
end

---装备随机属性2条目库属性数量检查
local function test_random_attr2( )
    local check_result = true

    for equip_id,equip_info in pairs ( EQUIPS ) do
        local random2 = equip_info.RandomAttr2
        local rule2 = equip_info.RandomAttrRule2
        local attr_max = 0
        if rule2 then
            attr_max = get_max_attr( rule2 )
        end
        if attr_max > 0 and not random2 then
            check_result = false
            c_err(sformat("equip_id is: %d RandomAttr2 is nil,but RandomAttr2_max is: %d",equip_id,attr_max))
        end
        if random2 and not check_attr( random2 )then
            check_result = false
            c_err(sformat("equip_id is: %d RandomAttr2 is not EQUIP_ATTRS !",equip_id))
        end
        if random2 and #random2 < attr_max then
            check_result = false
            c_err(sformat("equip_id is: %d len(RandomAttr2):%d < rule_max_num: %d",equip_id,#random2,attr_max))
        end
    end
    return check_result 
end

--装备隐藏属性条目和数量检查
local function test_hide_attr( )
    local check_result = true

    for equip_id,equip_info in pairs (EQUIPS) do
        local hide1 = equip_info.HideAttr1
        local rule = equip_info.HideAttrRule1
        local attr_max = 0
        if rule then
           attr_max = get_max_attr( rule )
        end
        if attr_max > 0 and not hide1 then
           check_result = false
           c_err(sformat("Equip_id is :%d HideAttr1 is nil,but HideAttr_max is: %d",equip_id,attr_max))
        end
        if hide1 and not check_attr( hide1 ) then
           check_result = false 
           c_err(sformat("Equip_id is :%d HideAttr1 is not EQUIP_ATTRS",equip_id))
        end
        if hide1 and #hide1 < attr_max then
           check_result = false
           c_err(sformat("Equip_id is: %d len(HideAttr1):%d < rule_max_num: %d",equip_id,#hide1,attr_max))
        end
    end 
    return check_result 
end

----装备精炼等级上限
local function test_refine_level( )
    local check_result = true
    for equip_id, equip_info in pairs (EQUIPS) do
        local max_refine = equip_info.MaxRefineLevel

        if not NO_CHECK_FIRST_BUY_EQUIPS[equip_id] and ITEMS[ equip_id ] and ITEMS[ equip_id ].UseLevel then
            item_level = ITEMS[ equip_id ].UseLevel

            if item_level > 0 and item_level < 30 then
                if max_refine ~= 10 then
                    check_result = false
                    c_err(sformat("Equip_id :%d  MaxRefineLevel: %d must 10",equip_id, max_refine))
                end
            end

            if item_level >= 30 and item_level <= 40 then
                if max_refine ~= 20 then
                    check_result = false
                    c_err(sformat("Equip_id :%d  MaxRefineLevel: %d must 20",equip_id, max_refine))
                end
            end

            if item_level >= 50 and item_level <= 70 then
                if max_refine ~= 30 then
                    check_result = false
                    c_err(sformat("Equip_id :%d  MaxRefineLevel: %d must 30",equip_id, max_refine))
                end
            end

            if item_level > 70 then
                if max_refine ~= 40 then
                    check_result = false
                    c_err(sformat("Equip_id :%d  MaxRefineLevel: %d must 40",equip_id, max_refine))
                end
            end

        end
    end
    return check_result 
end

----紫装不能再出逆天
----橙装出逆天的等级：60级，80级，100级，洗练条目上限7条
----橙装出神器的等级：60级，80级，100级, 洗练条目上限8条
local function test_attr_max()
    local check_result = true

    for equip_id,equip_info in pairs( EQUIPS ) do
        local attr_num = equip_info.AttrNumMax
        if attr_num then
                num_max = get_max_attr( attr_num)
        end

        if ITEMS[ equip_id ] and attr_num then
            uselevel = ITEMS[ equip_id ].UseLevel
            if ITEMS[ equip_id ].Quality == 4 and num_max > 6  then
                check_result = false
                c_err(sformat("Equip_id: %d Quality: %d AttrNumMax:%d  must < 7",equip_id,ITEMS[equip_id].Quality,num_max))
            end

            if ITEMS[ equip_id ].Quality == 5 and num_max > 6  then
                if uselevel ~= 60 and uselevel ~= 80 and uselevel ~= 100 then
                    check_result = false
                    c_err(sformat("Equip_id: %d AttrNumMax:%d  But Level:%d ~= (60,80,100)",equip_id,num_max,uselevel))
                end
            end
        end
    end
    return check_result 
end

---装备等级与宝石镶嵌等级检查
local function test_stone_max_level()
      check_result = true
 
      for equip_id, equip_info in pairs ( EQUIPS ) do
          if ITEMS[equip_id] and not NO_CHECK_FIRST_BUY_EQUIPS[equip_id]then
              local uselevel = ITEMS[equip_id].UseLevel
              local stone_max = equip_info.MaxStoneMountLevel

           --[[   if uselevel <= 20 and uselevel > 1 and stone_max ~= 6 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 6",equip_id,stone_max)
              end
            --]]
              if uselevel <= 30 and stone_max ~= 10 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 10",equip_id,stone_max) 
              end

              if uselevel == 40 and stone_max ~= 15 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 15",equip_id,stone_max) 
              end

              if uselevel == 50 and stone_max ~= 19 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 19",equip_id,stone_max) 
              end

              if uselevel == 60 and stone_max ~= 22 then
                  check_result = false
                  c_err("Equip_id :%d MaxStonLevel: %d  != 22",equip_id,stone_max) 
              end

              if uselevel == 70 and stone_max ~= 25 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 25",equip_id,stone_max) 
              end

              if uselevel == 80 and stone_max ~= 30 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 30",equip_id,stone_max) 
              end

              if uselevel == 85 and stone_max ~= 30 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 30",equip_id,stone_max) 
              end
              
              if uselevel == 90 and stone_max ~= 30 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 30",equip_id,stone_max) 
              end
             
              if uselevel == 95 and stone_max ~= 30 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 30",equip_id,stone_max) 
              end
             
              if uselevel > 95 and stone_max ~= 30 then
                  check_result = false
                  c_err("Equip_id :%d MaxStoneLevel: %d  != 30",equip_id,stone_max) 
              end
          end
      end
      return check_result 
end

---check 物品表每个物品的在背包所放分页
---1页：武器+手套（攻击类） 2页：盔甲+头盔+鞋子（防御类） 3页：腰带+戒指+项链和护符（饰品类）
---4页：变身技能书  5页：其余物品+宝石+各个系统材料等物品
local function test_item_page( )
    check_result = true

    for item_id , item_info in pairs ( ITEMS ) do
        item_type = item_info.ItemType
        use_type = item_info.UseType

        if item_type < 1 or item_type > 7 then
            check_result = false
            c_err(sformat("Item_id: %d item_type is: %d is wrong must between 1 ~ 6 ",item_id,item_type))
        end
        --if not EQUIPS[item_id] and use_type ~= 27 and item_type ~= 5 then
        --    check_result = false
        --    c_err(sformat("Item_id: %d  is not book  but item_type is: %d != 5",item_id,item_type))
        --end
        
        if not EQUIPS[item_id] and use_type == 27 and item_type ~= 4 then
            check_result = false
            c_err(sformat("Item_id: %d  is skill_book  but item_type is: %d != 4",item_id,item_type))
        end

        if EQUIPS[item_id]then
            equip_part = EQUIPS[item_id].EquipPart
            if equip_part == 6 or equip_part == 3 then
                if item_type ~= 1 then
                    check_result = false
                    c_err(sformat("Item_id: %d Part: %d item_type is: %d not equal 1",item_id,equip_part,item_type))
                end
            end
            if equip_part == 1 or equip_part == 2 or equip_part == 5 then
                if item_type ~= 2 then
                    check_result = false
                    c_err(sformat("Item_id: %d Part: %d item_type is: %d not equal 2",item_id,equip_part,item_type))
                end
            end
            if equip_part == 4 or equip_part == 7 or equip_part == 8 or equip_part ==9 then
                if item_type ~= 3 then
                    check_result = false
                    c_err(sformat("Item_id: %d Part: %d item_type is: %d not equal 3",item_id,equip_part,item_type))
                end
            end
        end
    end
    return check_result
end

---check五个品质装备绿色属性条数概率
--白装：100% 0条 
--绿装：60% 1条，10% 2条，30% 0条
--蓝装：100% 0条
--紫装：50% 1条 50% 0条
--橙装：50% 1条 50% 0条
local function test_RandomAttrRule1( )

     c_safe("start check RandomAttr1")
     for i,v in pairs(EQUIPS) do
         local RandomAttrRule1 = v.RandomAttrRule1
         local item = ITEMS[i] 
         if not NO_CHECK_EQUIP[ i ] and RandomAttrRule1 and item then
             if item.Quality ~= 2 and RandomAttrRule1 then
                 c_err("equip_id: "..i.." quality: "..item.Quality.." should not has randomAttr1")

             elseif item.UseLevel <50 and RandomAttrRule1 then
                 c_err("equip_id: "..i.." useLevel: "..item.UseLevel.." should not has randomAttr1")

             elseif not RandomAttrRule1 or not v.RandomAttr1 then
                 c_err("equip_id "..i.." should has randomAttr1")

             elseif #RandomAttrRule1 ~=2 or RandomAttrRule1[1][1] ~= 0 or RandomAttrRule1[2][1] ~= 1 then
                 c_err("equip_id "..i.." randomAttr1 config is wrong")
             end
         end
     end
     c_safe("end check randomAttr1")
     return true
end

---check五个品质装备蓝色属性条数概率
--白装：100% 0条 
--绿装：100% 0条
--蓝装：40% 1条 7.5% 2条 52.5% 0条
--紫装：50% 1条 50% 0条
--橙装：50% 1条 50% 0条

local function test_RandomAttrRule2( )

     c_safe("start check RandomAttr2")
     for i,v in pairs(EQUIPS) do
         local RandomAttrRule2 = v.RandomAttrRule2
         local item = ITEMS[i] 
         if not NO_CHECK_EQUIP[ i ] and RandomAttrRule2 and item then
             if item.Quality ~= 3 and RandomAttrRule2 then
                 c_err("equip_id: "..i.." quality: "..item.Quality.." should not has randomAttr2")

             elseif item.UseLevel <50 and RandomAttrRule2 then
                 c_err("equip_id: "..i.." useLevel: "..item.UseLevel.." should not has randomAttr2")

             elseif not RandomAttrRule2 or not v.RandomAttr2 then
                 c_err("equip_id "..i.." should has randomAttr2")

             elseif #RandomAttrRule2 ~=2 or RandomAttrRule2[1][1] ~= 0 or RandomAttrRule2[2][1] ~= 1 then
                 c_err("equip_id "..i.." randomAttr2 config is wrong")
             end
         end
     end
     c_safe("end check randomAttr2")
     return true
end

---check五个品质装备紫色属性条数概率
--白装：100% 0条 
--绿装：100% 0条
--蓝装：100% 0条
--紫装：100% 1条
--橙装：65% 1条 35% 2条

 function test_HideAttrRule1( )
     c_safe("start check HideAttr1")
     for i,v in pairs(EQUIPS) do
         local HideAttrRule1 = v.HideAttrRule1
         local item = ITEMS[i] 
         if not NO_CHECK_EQUIP[ i ] and  HideAttrRule1 and item then
             if item.Quality ~= 4 and HideAttrRule1 then
                 c_err("equip_id: "..i.." quality: "..item.Quality.." should not has hideAttr1")

             elseif item.UseLevel <50 and HideAttrRule1 then
                 c_err("equip_id: "..i.." useLevel: "..item.UseLevel.." should not has hideAttr1")

             elseif not HideAttrRule1 or not v.HideAttr1 then
                 c_err("equip_id "..i.." should has hideAttr1")

             elseif #HideAttrRule1 ~=2 or HideAttrRule1[1][1] ~= 0 or HideAttrRule1[2][1] ~= 1 then
                 c_err("equip_id "..i.." hideAttr1 config is wrong")
             end
         end
     end
     c_safe("end check HideAttr1")
     return true
end

local function test_equip_bind ()
    check_result = true

    for item_id, item_info in pairs ( ITEMS ) do
        local level = item_info.UseLevel
        local item_type = item_info.UseType 
        local bind_type = item_info.BindType
        local trade_id = item_info.TradeBindId 

        ----for ITEMS
        if trade_id and not ITEMS[trade_id] then
            check_result = false
            c_err(sformat("ITEM_ID :%d TradeBindId: %d is not in ITEMS",item_id,trade_id))
        end

        ----for EQUIPS        

        if EQUIPS[ item_id ] then
            local attr_max_info = EQUIPS[item_id].AttrNumMax
            local attr_max = 0
            if attr_max_info then
                attr_max = get_max_attr( attr_max_info )
            end

--[[
            if attr_max <= 6 and level < 30 then
                if bind_type ~= 1 then
                    check_result = false
                    c_err(sformat("equip_id: %d level: %d bind_type: %d must = 1",equip_id,level,bind_type))
                end
            end

            if attr_max <= 6 and level >= 30 then
                if bind_type == 1 then
                    check_result = false
                    c_err(sformat("equip_id :%d level: %d bind_type: %d is wrong !",item_id,level,bind_type))
                end
            end

            if attr_max == 7 and bind_type ~= 1 then
                check_result = false
                c_err(sformat("equip_id: %d attr_max :%d must bind!",item_id,attr_max))
            end
  --]]      
         end
    end
    return check_result 
end

local function test_reborn_RecommendGold()
     check_result = true
     local equip_attrs_cfg = EQUIP_ATTRS
     
     for attrs_index, attrs in pairs ( equip_attrs_cfg ) do
         reborn_attr = attrs.RebornDataZone
         reborn_recommendgold = attrs.RecommendGold
         
         if reborn_attr and reborn_recommendgold then
             if #reborn_attr ~= #reborn_recommendgold[1] then
                 check_result = false
                 c_err(sformat(" attrs_id : %d #RebornDataZone: %d ~= #RecommendGold: %d",attrs_index,#reborn_attr,#reborn_recommendgold[1]))
             end
         end
     end
     return check_result 
end

local function test_equip_orange()
    local check_result = true
    for equip_id , equip_info in pairs ( EQUIPS ) do
        local orange_type = equip_info.OrangeAscendType
        local num_max = equip_info.AttrNumMax
        
        local attr_max = 0
        if num_max then
            attr_max = get_max_attr( num_max )
        end

        if ITEMS [equip_id] then
            quality = ITEMS[equip_id].Quality 
        end

        if quality == 5 and not ORANGE_EQUIP_ASCEND_MATERIAL[ equip_id] then
           check_result = false
           --add by kent  
           --c_err(sformat("equip_id: %d quality is :%d not found ORANGE_EQUIP_ASCEND_MATERIAL",equip_id, quality))
           --
        end

        if quality ==5 and not orange_type then
            check_result = false
            c_err(sformat ("equip_id: %d quality is :%d but OrangeAscendType is nil !", equip_id, quality))
        end
        
        if quality ==5 and not check_orange_fix_attr(equip_id) then
            c_err("equip_id: "..equip_id.." check fix attr failed")
        end

        if orange_type and quality == 5 then
            if orange_type < 0 or orange_type > 3 then
                check_result = false
                c_err(sformat ("equip_id: %d orange_type:%d must 0 ~ 3 !", equip_id, orange_type))
            end
            
            if attr_max == 6 and orange_type ~= 0 then
                check_result = false
                c_err(sformat("equip_id: %d attr_max is: %d, but orange_type is :%d not equal 0 ", equip_id , attr_max , orange_type))
            end
--[[
            if attr_max == 7 and orange_type ~= 1 then
                check_result = false
                c_err ( sformat ("equip_id: %d attr_max is: %d, but orange_type is :%d not equal 1", equip_id , attr_max , orange_type))
            end

            if attr_max == 9 and orange_type < 2 then
                check_result = false
                c_err ( sformat ("equip_is: %d attr_max is : %d but orange_type < 2 ",equip_id, attr_max ))
            end
--]]
        end
    end
    return check_result
end

function test_item_bind ( )
       check_result = true
       
       for item_id , item_info in pairs ( ITEMS ) do
           local item_type = item_info.UseType       --- 使用类型
           local bind_type = item_info.BindType      --- 绑定类型
           local trade_id  = item_info.TradeBindId   --- 交易后ID
           local quality = item_info.Quality         --- 物品品质
           
           -- 策划已废弃类型27、29,并将类型5改为绑定   do by abel 2018-5-24
           -- if item_type and item_type == 27 or item_type == 29 or item_type == 5  then   --技能书/残页+符文材料始终不绑定
               -- if bind_type ~=3 then
                   -- check_result = false
                   -- c_err (sformat ("item : %d is skill_book but bind_type is: %d  ~=3 !", item_id, bind_type ))
               -- end
               -- if trade_id then
                  -- check_result = false
                  -- c_err(sformat("item: %d is skill_book but get trade_id is: %d", item_id , trade_id))
               -- end
           -- end

           if item_type and item_type == 28 or item_type == 30 then       ---蓝紫变身、附魔石交易后绑定
               if quality < 5 and  bind_type == 3 then
                   if not trade_id then
                       check_result = false
                       c_err (sformat("item: %d is transform_quality :%d  bind_type is 3 but not trade_id", item_id ,quality))
                   end
                   
                   if trade_id then
                       if not ITEMS[trade_id] or ITEMS[trade_id].Quality ~= quality then
                           check_result = false
                           c_err (sformat ("item: %d and trade_id :%d  Quality is not equal! ",item_id,trade_id ))
                       end
                   end

                   if quality == 5 and bind_type ~= 1 then
                       check_result = false
                       c_err(sformat ("item: %d is orange transform bind_type is :%d must 5 !",item_id , bind_type))
                   end
               end
           end
       end
       return check_result
end

local TEST_FUNC = {
     [1] = test_part,
     [2] = test_pack_max,
     [3] = test_attr_num,
     [4] = test_equip_level,
     [5] = test_fix_attr,
     [6] = test_random_attr1,
     [7] = test_random_attr2,
     [8] = test_hide_attr,
     [9] = test_refine_level,
     [10] = test_stone_max_level,
     [11] = test_item_page,
     [12] = test_RandomAttrRule1,
     [13] = test_RandomAttrRule2,
     [14] = test_HideAttrRule1,
     [16] = test_reborn_RecommendGold,
     [17] = test_equip_orange,
     [18] = test_item_bind,
     [19] = test_equip_bind,
    -- [20] = test_attr_max,
}

function test()
    local check_result = true
    c_safe(sformat("EQUIPS_config Check start!!"))

    for _,func in pairs(TEST_FUNC) do
        if func then
           if not func() then
               check_result = false
           end
        end
    end
    if not check_result then
    c_safe(sformat("EQUIPS_config Check not pass!!"))
    end
    c_safe(sformat("EQUIPS_config Check over !!"))
    return check_result 
end

function check_orange_fix_attr(_equip_id)
    local equip = EQUIPS[_equip_id]
    local fix_attr = equip["FixAttr"]
    for _,attr_index in pairs(fix_attr) do
        local attr_info = EQUIP_ATTRS[attr_index]
        local common_data_zone = attr_info["CommonDataZone"][1]
        if common_data_zone[1] ~= 10000 then
            c_err("equip_id: ".._equip_id.."attr_index "..attr_index.." fix attr rate not right")
            return false
        end
        if common_data_zone[2] ~= common_data_zone[3] then
            c_err("equip_id: ".._equip_id.."attr_index "..attr_index.." attr value is not stable")
            return false
        end
    end

    return true
end


