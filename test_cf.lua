function no_tmp_slot()
    local a = 1
    a = a+2
    a = a+3
end

function tmp_slot()
    local a = 1
    a = a+2+3
end

function multi_pass()
    local a = 1
    a = a+2
    a = a+3
    a = a+2+3
end

function no_opt()
    local a, b = 1, 2
    a = a+1
    b = b+a
end

--[[
function commutate()
    local a = 1
    a = 2+a
    a = a+3
end

function sub1()
    local a = 1
    a = a-2
    a = a-3
end

function sub2()
    local a = 1
    a = a-2
    a = 3-a
end

function sub3()
    local a = 1
    a = 2-a
    a = 3-a
end

function sub4()
    local a = 1
    a = 2-a
    a = a-3
end

function sub5()
    local a = 1
    a = a-2
    a = a+3
end

function sub6()
    local a = 1
    a = a-2
    a = 3+a
end

function sub7()
    local a = 1
    a = 2-a
    a = a+3
end

function sub8()
    local a = 1
    a = 2-a
    a = a+4
end

function sub9()
    local a = 1
    a = a+2
    a = a-3
end

function sub10()
    local a = 1
    a = 2+a
    a = a-3
end

function sub11()
    local a = 1
    a = a+2 
    a = 3-a
end

function sub12()
    local a = 1
    a = 2+a
    a = 3-a
end
--]]
