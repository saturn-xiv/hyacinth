add_rules("mode.debug", "mode.release")
-- "mysqlpp", "mysql" "mariadb-connector-c",
add_requires("boost", "openssl3", "libpq", "sqlite3")
-- add_requires("mariadb-connector-c", {configs = {ssl = false}})

target("hyacinth")
    set_kind("binary")
    set_languages("c11", "c++17")
    add_syslinks("dl")
    add_ldflags("-pthread")
    add_packages("boost", "openssl3", "libpq",  "sqlite3")
    add_files("src/*.cpp")    
    add_includedirs("./include")
    

