--
-- premake5 file to build RecastDemo
-- http://premake.github.io/
--
require "lib"

local action = _ACTION or ""
local outdir = action


WorkSpaceInit  "samples"


Project "echo_server"
	includedirs { 
		"../external/",
		"../",
	}

	SrcPath { 
		"../samples/echo_server/**",   
		"../samples/com/**",   
	}

    
Project "echo_client"
	includedirs { 
		"../external/",
		"../",
	}

	SrcPath { 
		"../samples/echo_client/**", 
		"../samples/com/**",     
	}

    