function no_tmp_slot()
    local a
    a = a+2
    a = a+3
end

function tmp_slot()
    local a
    a = a+2+3
end

function multi_pass()
    local a
    a = a+2
    a = a+3
    a = a+2+3
end

function no_opt()
    local a, b
    a = a+1
    b = b+a
end

function commutate()
    local a
    a = 2+a
    a = a+3
end

function sub1()
    local a
    a = a-2
    a = a-3
end

function sub2()
    local a
    a = a-2
    a = 3-a
end

function sub3()
    local a
    a = 2-a
    a = 3-a
end

function sub4()
    local a
    a = 2-a
    a = a-3
end

function sub5()
    local a
    a = a-2
    a = a+3
end

function sub6()
    local a
    a = a-2
    a = 3+a
end

function sub7()
    local a
    a = 2-a
    a = a+3
end

function sub8()
    local a
    a = 2-a
    a = a+3
end

function sub9()
    local a
    a = a+2
    a = a-3
end

function sub10()
    local a
    a = 2+a
    a = a-3
end

function sub11()
    local a
    a = a+2 
    a = 3-a
end

function sub12()
    local a
    a = 2+a
    a = 3-a
end

function m_no_tmp_slot()
    local a
    a = a*2
    a = a*3
end

function m_tmp_slot()
    local a
    a = a*2*3
end

function m_multi_pass()
    local a
    a = a*2
    a = a*3
    a = a*2*3
end

function m_no_opt()
    local a, b
    a = a*1
    b = b*a
end

function m_commutate()
    local a
    a = 2*a
    a = a*3
end

function div1()
    local a
    a = a/2
    a = a/3
end

function div2()
    local a
    a = a/2
    a = 3/a
end

function div3()
    local a
    a = 2/a
    a = 3/a
end

function div4()
    local a
    a = 2/a
    a = a/3
end

function div5()
    local a
    a = a/2
    a = a*3
end

function div6()
    local a
    a = a/2
    a = 3*a
end

function div7()
    local a
    a = 2/a
    a = a*3
end

function div8()
    local a
    a = 2/a
    a = a*3
end

function div9()
    local a
    a = a*2
    a = a/3
end

function div10()
    local a
    a = 2*a
    a = a/3
end

function div11()
    local a
    a = a*2 
    a = 3/a
end

function div12()
    local a
    a = 2*a
    a = 3/a
end
