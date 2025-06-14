local key = KEYS[1];
local increment = tonumber(ARGV[1] or 1);
local current = tonumber(redis.call('get', key) or '0');

if not increment or not current then
    return nil;
end;

local new_value = current + increment;

redis.call('set', key, new_value);
return new_value;