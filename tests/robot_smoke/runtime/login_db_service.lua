module("db_service", package.seeall)

local accounts = {}

local function account_info(_account_id)
    local value = accounts[_account_id]
    if not value then
        return nil
    end

    return {
        account_id_ = _account_id,
        machine_code_ = value.machine_code_,
        login_token_ = value.login_token_,
        queue_free_ = 1,
        forbid_login_ = 0,
        channel_account_ = value.channel_account_,
        channel_id_ = value.channel_id_,
        channel_label_ = value.channel_label_,
        is_prev_test_user_ = true,
        return_bind_diamond_ = 0,
        return_non_bind_diamond_ = 0,
        device_name_ = value.device_name_,
        system_name_ = value.system_name_,
    }
end

function init()
    return true
end

function on_exit()
    accounts = {}
end

function is_new_account(_account_id)
    return accounts[_account_id] == nil
end

function update_account_info(_account_id, _mcode, _login_token, _channel_account, _channel_id, _channel_label, _device_name, _system_name)
    accounts[_account_id] = {
        machine_code_ = _mcode,
        login_token_ = _login_token,
        channel_account_ = _channel_account,
        channel_id_ = _channel_id,
        channel_label_ = _channel_label,
        device_name_ = _device_name,
        system_name_ = _system_name,
    }
    return 0
end

function invalid_account_token(_account_id)
    return 0
end

function get_account_info(_account_id)
    local value = account_info(_account_id)
    return value ~= nil, value
end

function verify_account(_account_id, _login_token)
    local value = account_info(_account_id)
    return value ~= nil and value.login_token_ == _login_token
end

function get_active_info_by_account_id(_account_id, _server_id)
    return true
end

function get_active_info_by_code(_active_code)
    return true, 1, 0
end

function update_active_info(_account_id, _active_code)
    return true
end

function get_account_info_by_machine_code(_machine_code)
    return {}
end

function get_machine_forbid_end_time(_machine_code)
    return 0
end

function get_all_forbid_account()
    return {}
end

function update_forbid_machine_info(_machine_code, _forbid_end_time)
    return true
end

function update_forbid_info(_account_id, _forbid_end_time)
    if not accounts[_account_id] then
        return false, "smoke account not found"
    end
    return true
end

function get_account_with_same_machine_code(_account_id)
    local value = account_info(_account_id)
    return value and value.machine_code_ or ""
end

function clear_return_diamond(_account_id, _game_server_id, _player_id)
    return true
end

function verify_and_save_account_when_enter_login(_dpid, _platform, _version, _account_id, _mcode, _sdk_time_stamp, _sdk_sign, _channel_account, _channel_id, _channel_label, _server_id, _device_name, _system_name, _active_code)
    local is_new = is_new_account(_account_id)
    local login_token = 24681357
    update_account_info(_account_id, _mcode, login_token, _channel_account, _channel_id, _channel_label, _device_name, _system_name)
    return true, login_code.LOGIN_CERTIFY_OK, login_token, is_new
end

function verify_account_when_enter_game(_account_id, _login_token)
    local value = account_info(_account_id)
    if not value then
        return login_code.LOGIN_ACCOUNT_NOT_FOUND, 0
    end
    if value.login_token_ ~= _login_token then
        return login_code.LOGIN_ACCOUNT_TIMEOUT, 0
    end
    return login_code.LOGIN_CERTIFY_OK, 1
end
