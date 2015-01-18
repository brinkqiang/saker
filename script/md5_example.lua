
function md5_example()
    saker.log(LOG_INFO, "This is md5_example")
    local teststr = "abcdefghijklmn"
    saker.log(LOG_INFO, "MD5("..teststr..") = "..saker.md5(teststr,string.len(teststr)))
    return true
end

saker.register("md5_example", "md5_example", PROP_ONCE)
