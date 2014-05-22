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
