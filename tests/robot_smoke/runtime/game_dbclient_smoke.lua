function send_certify_to_db(_dpid, _account_id)
    if AppConfig and AppConfig.SmokeMode == 1 then
        gamesvr.send_certify_code_to_client(_dpid, login_code.CERTIFY_OK)
        c_log("[dbclient] SmokeMode certify accepted account:%d", _account_id)
        return
    end

    local client_ip_str = g_gamesvr:c_get_player_ip(_dpid)
    local ar = g_ar
    ar:flush_before_send(msg.DB_TYPE_CERTIFY)
    ar:write_ulong(_dpid)
    ar:write_double(_account_id)
    ar:write_string(client_ip_str)
    ar:send_one_ar(g_dbclient, 0)
end
