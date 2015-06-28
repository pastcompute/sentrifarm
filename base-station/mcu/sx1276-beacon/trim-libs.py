from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

env.Replace(LIBS=["pp", "main", "wpa", "lwip", "net80211", "phy", "hal", "gcc"])

# uncomment line below to see environment variables, or instead, use --target envdump
#print env.Dump()
