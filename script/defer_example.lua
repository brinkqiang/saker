
-- saker退出时执行
function defer_example()
    saker.log(LOG_INFO, "This is defer_example")
    return true
end

saker.register("defer_example", "defer_example", PROP_DEFER)
