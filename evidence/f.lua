local function f(a, b, c)
  local x = a + b * c
  return x
end
local function fold()
  local y = 1 + 2 * 3
  return y
end
return f, fold
