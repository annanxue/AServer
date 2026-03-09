AppConfig =
{
    Daemon = 0,
    TimerSize = 1024,

    Log =
    {
        ErrLogFile = "./app_base_test_error.log",
        LogPathPrefix = "./log/test",
        LogLevel = 0,
        ErrLevel = 4,
        LogLevel = 2,
        TraceLevel = 2,
    },

    Network =
    {
        mempooler_socket = 1000,
        mempooler_buffer = 2000,
        mempooler_bufque = 3000,
        timeout = 1000,
        port = 100001,
        core_ip = "127.0.0.1",
        max_users = 10,
    },

    Lua =
    {
        path = "./",
        main_file = "main.lua",
    },

    MyString_Alloc = 16384,
}
