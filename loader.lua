-- This script is part of lua2exe
-- Loader for embedded scripts
local load2 =  loadembed
local names  = embeddedfiles
local function loader (name)
if names[name] then 
return load2(name, names[name])
else return 'No embedded module \''..name ..'\''
end end
loadembed = nil
embeddedfiles = nil
table.insert(package.loaders, 2, loader)
