AppConfig = 
{
    Daemon = 0,         --是否(1/0)后台模式启动
    TimerSize = 1024,   --Timer预生成的内存块大小

    Log = 
    {
        ErrLogFile = "./log/app_base_test_error.log",
        LogPathPrefix = "./log",
        LogLevel = 0,
        ErrLevel = 4,
        LogLevel = 2,
        TraceLevel = 2,
    },

    Network =
    {
        mempooler_socket = 1000,
        mempooler_buffer = 2000,
        mempooler_bufnode = 2000,
        mempooler_bufque = 3000,
        flow_stats_time = 60,
    },

    Lua = 
    {
        path = "./",
        main_file = "main.lua", 
    },

    DbSvr =
    {
        MaxCon = 4096,              -- max socket = 4096<<2
        Port = 9228,
        Crypt = 1,
        PollWaitTime = 100,
        ProcThreadNum = 4,
    },

    MysqlCfg = "./mysql.lua",
}

