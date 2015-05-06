
function args_example(args)
    if type(args) ~= "table" then
      return false, "Wrong args"
    end
    for k,v in pairs(args) 
    do
    print(k..":"..v)
    end
    return true
end

saker.register("args_example", "args_example", PROP_PASSIVITY)