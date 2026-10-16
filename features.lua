-- Protocol profile for the Old FasterOT 7.72 server.
-- The built-in 7.72 feature set matches its packet layout. 
ClientFeatures = {
    selectedProfile = "oldfasterot772",
  
    default = {
        protocol = 772,
        host = "35.198.28.170",
        port = 7171,
    },

    serverProfiles = {
        ["35.198.28.170:7171"] = "oldfasterot772",
        ["108.165.179.106:7171"] = "oldfasterot772",
        ["127.0.0.1:7171"] = "oldfasterot772",
        ["localhost:7171"] = "oldfasterot772",
    },
 
    profiles = {
        oldfasterot772 = {
            description = "Old FasterOT 7.72",
            protocol = 772,
            clientVersion = 772,
            fileVersion = 772,
            inheritDefaults = true,
            features = {
                -- The server sends these fields even with protocol number 772.
                GamePlayerAddons = true,
                GamePlayerMounts = true,
                GameNewOutfitProtocol = true,
                GameEnvironmentEffects = true,
                -- Runes carry their charges as a count byte (see sendLogin).
                GameRuneCharges = true,
                GameDoublePlayerGoodsMoney = true,
                GameDoubleShopBuyAmount = true,
                GameDoubleShopSellAmount = true,
            }, 
        },
    },
}
