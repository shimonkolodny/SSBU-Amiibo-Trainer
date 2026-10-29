#include "ssbu_loadouts.hpp"
#include <algorithm>
#include <cctype>

namespace ssbu {

static std::string ToLower(const std::string& s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

const char* SpiritTypeToString(SpiritType t) {
    switch (t) {
        case SpiritType::Neutral: return "Neutral";
        case SpiritType::Attack:  return "Attack";
        case SpiritType::Defense: return "Defense";
        case SpiritType::Grab:    return "Grab";
        default:                  return "Neutral";
    }
}

// Complete 255-entry SSBU Support Spirit Ability table (0xEC, 0xED, 0xEE)
// Grounded in jozz024/smash-amiibo-editor resources/abilities.txt and MiDe's SSBU Amiibo Bin Guide
const std::vector<SupportSpiritAbility>& GetSupportSpiritCatalog() {
    static const std::vector<SupportSpiritAbility> kCatalog = {
        {0,   "None", 0, true, "Empty spirit slot"},
        {1,   "Move Speed Up", 1, true, "Increases ground movement speed (L 1)"},
        {2,   "Hyper Smash Attacks", 1, true, "Increases smash attack charge speed and damage (L 1)"},
        {3,   "Reserved_03", 1, false, "Unused internal slot 3"},
        {4,   "Jump Up", 1, true, "Increases jump height (L 1)"},
        {5,   "Additional Midair Jump", 2, true, "Grants an extra midair jump (L 2)"},
        {6,   "Lifesteal", 3, false, "Heals when dealing damage to opponents (U 3)"},
        {7,   "Defense Up", 1, false, "Increases defense stat multiplier"},
        {8,   "Easier Dodging", 1, true, "Increases dodge invincibility window (L 1)"},
        {9,   "Easier Perfect Shield", 1, true, "Widens parry timing window (L 1)"},
        {10,  "Super Armor", 3, true, "Prevents flinching from weak and medium attacks (L 3)"},
        {11,  "Slow Super Armor", 2, true, "Grants Super Armor at reduced movement speed (L 2)"},
        {12,  "Trade-Off Attacks Up", 1, true, "Starts match with 30% damage and higher attack power (L 1)"},
        {13,  "Trade-Off Defense Up", 1, true, "Starts match with 30% damage and higher defense (L 1)"},
        {14,  "Trade-Off Speed Up", 1, true, "Starts match with 30% damage and higher speed (L 1)"},
        {15,  "Trade-Off Ability Up", 1, true, "Starts match with 30% damage and boosted stats (L 1)"},
        {16,  "Critical-Health Attack Up", 1, true, "Boosts attack power at high damage percent (L 1)"},
        {17,  "Critical-Health Defense Up", 1, true, "Boosts defense at high damage percent (L 1)"},
        {18,  "Critical-Health Stats Up", 1, true, "Boosts attack, defense, and speed at high percent (L 1)"},
        {19,  "Critical Immunity", 2, true, "Grants brief invincibility when reaching critical health (L 2)"},
        {20,  "Autoheal", 2, true, "Recovers 3% damage every 5 seconds (L 2)"},
        {21,  "Poison Immunity", 2, true, "Negates poison floor hazards (L 2)"},
        {22,  "Poison Damage Reduced", 1, true, "Reduces damage taken from poison hazards (L 1)"},
        {23,  "Poison Heals", 3, true, "Reverses poison damage into healing (L 3)"},
        {24,  "Lava-Floor Immunity", 2, true, "Negates lava floor hazards (L 2)"},
        {25,  "Sticky-Floor Immunity", 2, true, "Negates sticky floor hazards (L 2)"},
        {26,  "Beam Sword Equipped", 1, true, "Starts match holding a Beam Sword (L 1)"},
        {27,  "Lip's Stick Equipped", 1, true, "Starts match holding Lip's Stick (L 1)"},
        {28,  "Star Rod Equipped", 1, true, "Starts match holding a Star Rod (L 1)"},
        {29,  "Ore Club Equipped", 3, true, "Starts match holding an Ore Club (L 3)"},
        {30,  "Home-Run Bat Equipped", 3, false, "Starts match holding a Home-Run Bat"},
        {31,  "Ray Gun Equipped", 2, true, "Starts match holding a Ray Gun (L 2)"},
        {32,  "Super Scope Equipped", 2, true, "Starts match holding a Super Scope (L 2)"},
        {33,  "Gust Bellows Equipped", 2, false, "Starts match holding a Gust Bellows"},
        {34,  "Drill Equipped", 1, true, "Starts match holding a Drill (L 1)"},
        {35,  "Green Shell Equipped", 1, true, "Starts match holding a Green Shell (L 1)"},
        {36,  "Poke Ball Equipped", 2, false, "Starts match holding a Poke Ball"},
        {37,  "Reserved_37", 1, false, "Unused internal slot 37"},
        {38,  "Back Shield Equipped", 1, true, "Starts match with a Back Shield (L 1)"},
        {39,  "Bunny Hood Equipped", 1, true, "Starts match wearing a Bunny Hood (L 1)"},
        {40,  "Made of Metal", 1, true, "Starts match in Metal form (L 1)"},
        {41,  "Mouthful of Curry", 2, true, "Starts match with Superspicy Curry active (L 2)"},
        {42,  "Franklin Badge Equipped", 2, true, "Starts match wearing a Franklin Badge to reflect projectiles (L 2)"},
        {43,  "Hammer Equipped", 3, false, "Starts match holding a Hammer"},
        {44,  "Fairy Bottle Equipped", 1, true, "Starts match holding a Fairy Bottle (L 1)"},
        {45,  "Fire Flower Equipped", 1, true, "Starts match holding a Fire Flower (L 1)"},
        {46,  "Freezie Equipped", 1, true, "Starts match holding a Freezie (L 1)"},
        {47,  "Ramblin' Evil Mushroom Equipped", 1, true, "Starts match holding a Ramblin' Evil Mushroom (L 1)"},
        {48,  "Killing Edge Equipped", 1, true, "Starts match holding a Killing Edge (L 1)"},
        {49,  "Reserved_49", 1, false, "Unused internal slot 49"},
        {50,  "Physical Attack Up", 1, true, "Increases punch, kick, and bite damage (L 1)"},
        {51,  "Weapon Attack Up", 1, true, "Increases sword, hammer, and weapon damage (L 1)"},
        {52,  "Fist Attack Up", 1, true, "Increases punch and elbow attack damage (L 1)"},
        {53,  "Foot Attack Up", 1, true, "Increases kick and knee attack damage (L 1)"},
        {54,  "Aura Attack Up", 1, true, "Increases Aura attack damage (L 1)"},
        {55,  "Magic Attack Up", 1, true, "Increases magic attack damage (L 1)"},
        {56,  "PSI Attack Up", 1, true, "Increases PSI attack damage (L 1)"},
        {57,  "Reserved_57", 1, false, "Unused internal slot 57"},
        {58,  "Fire & Explosion Attack Up", 1, true, "Increases fire and explosion damage (L 1)"},
        {59,  "Reserved_59", 1, false, "Unused internal slot 59"},
        {60,  "Electric Attack Up", 1, true, "Increases electric attack damage (L 1)"},
        {61,  "Energy-Shot Attack Up", 1, true, "Increases energy projectile damage (L 1)"},
        {62,  "Water & Ice Attack Up", 1, true, "Increases water and freezing attack damage (L 1)"},
        {63,  "Magic Resist Up", 1, true, "Reduces damage taken from magic attacks (L 1)"},
        {64,  "PSI Resist Up", 1, true, "Reduces damage taken from PSI attacks (L 1)"},
        {65,  "Reserved_65", 1, false, "Unused internal slot 65"},
        {66,  "Fire/Explosion Resist Up", 1, true, "Reduces damage taken from fire and explosions (L 1)"},
        {67,  "Reserved_67", 1, false, "Unused internal slot 67"},
        {68,  "Reserved_68", 1, false, "Unused internal slot 68"},
        {69,  "Electric Resist Up", 1, true, "Reduces damage taken from electric attacks (L 1)"},
        {70,  "Energy-Shot Resist Up", 1, true, "Reduces damage taken from energy projectiles (L 1)"},
        {71,  "Reserved_71", 1, false, "Unused internal slot 71"},
        {72,  "Water/Freezing Resist Up", 1, true, "Reduces damage taken from water and ice attacks (L 1)"},
        {73,  "Aura Resist Up", 1, true, "Reduces damage taken from Aura attacks (L 1)"},
        {74,  "Zap-Floor Immunity", 2, true, "Negates electrified floor hazards (L 2)"},
        {75,  "Slumber Immunity", 1, true, "Prevents sleep status effects (L 1)"},
        {76,  "Ice-Floor Immunity", 2, true, "Negates icy floor slipping and freezing (L 2)"},
        {77,  "Falling Immunity", 1, true, "Prevents tripping (L 1)"},
        {78,  "Bury Immunity", 1, true, "Prevents being buried in the ground (L 1)"},
        {79,  "Braking Ability Up", 1, true, "Increases ground traction and stopping speed (L 1)"},
        {80,  "Mobility Up", 1, false, "Boosts overall air and ground mobility"},
        {81,  "Landing Lag Down", 1, true, "Reduces landing lag on aerial attacks (L 1)"},
        {82,  "Lightweight", 1, true, "Increases mobility and jump height while reducing weight (L 1)"},
        {83,  "Shield Damage Up", 1, true, "Deals extra damage to opponent shields (L 1)"},
        {84,  "Air Attack Up", 1, true, "Increases aerial attack damage (L 1)"},
        {85,  "Air Defense Up", 1, true, "Reduces damage taken while airborne (L 1)"},
        {86,  "Neutral Special Up", 1, true, "Increases neutral special damage (L 1)"},
        {87,  "Side Special Up", 1, true, "Increases side special damage (L 1)"},
        {88,  "Up Special Up", 1, true, "Increases up special damage (L 1)"},
        {89,  "Down Special Up", 1, true, "Increases down special damage (L 1)"},
        {90,  "Strong Throw", 1, true, "Increases throw damage and knockback (L 1)"},
        {91,  "Unflinching Charged Smashes", 2, true, "Grants super armor while charging smash attacks (L 2)"},
        {92,  "Toss & Meteor", 1, true, "Increases up-throw and meteor smash power (L 1)"},
        {93,  "Reserved_93", 1, false, "Unused internal slot 93"},
        {94,  "Critical Hit Up", 1, true, "Grants a 5% chance to deal 2x damage (L 1)"},
        {95,  "Swimmer", 1, true, "Allows unlimited swimming in water (L 1)"},
        {96,  "Shield Durability Up", 1, true, "Increases maximum shield health (L 1)"},
        {97,  "Improved Escape", 1, true, "Makes mashing out of grabs, buries, and stuns faster (L 1)"},
        {98,  "Reserved_98", 1, false, "Unused internal slot 98"},
        {99,  "Reserved_99", 1, false, "Unused internal slot 99"},
        {100, "Reserved_100", 1, false, "Unused internal slot 100"},
        {101, "Battering Items Up", 1, true, "Increases damage dealt with battering items (L 1)"},
        {102, "Shooting Items Up", 1, true, "Increases ammo and power of shooting items (L 1)"},
        {103, "Thrown Items Up", 1, true, "Increases speed and power of thrown items (L 1)"},
        {104, "KOs Heal Damage", 2, true, "Heals damage whenever scoring a KO (L 2)"},
        {105, "Invincibility after Eating", 2, true, "Grants brief invincibility after eating food (L 2)"},
        {106, "Stats Up after Eating", 1, true, "Boosts attack, defense, and speed after eating (L 1)"},
        {107, "Reserved_107", 1, false, "Unused internal slot 107"},
        {108, "First-Strike Advantage", 2, true, "Grants brief invincibility when landing the first hit of a match (L 2)"},
        {109, "Reserved_109", 1, false, "Unused internal slot 109"},
        {110, "Running Start", 2, true, "Boosts movement speed at the start of a match (L 2)"},
        {111, "Reserved_111", 1, false, "Unused internal slot 111"},
        {112, "Fast Final Smash Meter", 2, true, "Charges Final Smash meter faster (L 2)"},
        {113, "Instadrop", 2, true, "Enables damaging fast-fall drop (L 2)"},
        {114, "Healing Shield", 2, true, "Heals fighter when shielding attacks (L 2)"},
        {115, "Reserved_115", 1, false, "Unused internal slot 115"},
        {116, "Reserved_116", 1, false, "Unused internal slot 116"},
        {117, "Floaty Jumps", 1, true, "Reduces gravity while jumping and falling (L 1)"},
        {118, "Reserved_118", 1, false, "Unused internal slot 118"},
        {119, "Irreversible Controls", 2, true, "Prevents control-reversal status effects (L 2)"},
        {120, "Recovery Items Up", 1, false, "Increases healing received from recovery items"},
        {121, "Transformation Duration Up", 1, true, "Extends duration of fighter transformations and power-ups (L 1)"},
        {122, "Undamaged Attack Up", 1, true, "Boosts attack power while at 0% damage (L 1)"},
        {123, "Undamaged Speed Up", 1, true, "Boosts movement speed while at 0% damage (L 1)"},
        {124, "Undamaged Attack & Speed Up", 1, true, "Boosts attack and speed while at 0% damage (L 1)"},
        {125, "Reserved_125", 1, false, "Unused internal slot 125"},
        {126, "Reserved_126", 1, false, "Unused internal slot 126"},
        {127, "Edge Grab Up", 1, false, "Increases ledge snap range"},
        {128, "Impact Run", 1, true, "Deals damage when dashing into opponents (L 1)"},
        {129, "Reserved_129", 1, false, "Unused internal slot 129"},
        {130, "Lava-Floor Resist", 1, true, "Reduces damage taken from lava floors (L 1)"},
        {131, "Item Gravitation", 1, true, "Pulls nearby items toward the fighter (L 1)"},
        {132, "Reserved_132", 1, false, "Unused internal slot 132"},
        {133, "Chance of Double Final Smash", 2, true, "Chance to use Final Smash twice in a row (L 2)"},
        {134, "Double Final Smash", 3, true, "Guarantees a second Final Smash after the first (L 3)"},
        {135, "Reserved_135", 1, false, "Unused internal slot 135"},
        {136, "Reserved_136", 1, false, "Unused internal slot 136"},
        {137, "Metal and Giant", 3, false, "Starts match both Metal and Giant"},
        {138, "Giant", 2, true, "Starts match as a Giant fighter (L 2)"},
        {139, "Dash Attack Up", 1, true, "Increases dash attack damage and speed (L 1)"},
        {140, "Armor Knight", 2, true, "Greatly boosts defense and attack while reducing speed (L 2)"},
        {141, "Reserved_141", 1, false, "Unused internal slot 141"},
        {142, "Energy Shot Attack/Resistance Up", 2, true, "Boosts energy projectile damage and resistance (L 2)"},
        {143, "Hammer Duration Up", 1, true, "Extends duration of Hammer and Golden Hammer items (L 1)"},
        {144, "Boomerang Equipped", 1, true, "Starts match holding a Boomerang (L 1)"},
        {145, "Item Attack Up", 1, false, "Boosts damage of all item attacks"},
        {146, "Reserved_146", 1, false, "Unused internal slot 146"},
        {147, "Reserved_147", 1, false, "Unused internal slot 147"},
        {148, "Reserved_148", 1, false, "Unused internal slot 148"},
        {149, "Reserved_149", 1, false, "Unused internal slot 149"},
        {150, "Reserved_150", 1, false, "Unused internal slot 150"},
        {151, "Perfect-Shield Reflect", 1, true, "Parrying reflects projectiles with boosted speed (L 1)"},
        {152, "Weapon Attack & Move Speed Up", 2, true, "Boosts weapon attack damage and movement speed (L 2)"},
        {153, "Shooting Attack Up", 1, true, "Boosts fighter projectile and shooting attacks (L 1)"},
        {154, "Charge Speed & Power Up", 2, false, "Increases charge speed and power of chargeable moves"},
        {155, "Reserved_155", 1, false, "Unused internal slot 155"},
        {156, "Screen-Flip Immunity", 2, true, "Negates screen-flip stage hazards (L 2)"},
        {157, "Fog Immunity", 2, true, "Clears fog stage hazards (L 2)"},
        {158, "Gravity-Change Immunity", 2, true, "Negates gravity-change stage hazards (L 2)"},
        {159, "Stamina Up", 1, true, "Increases starting HP in Stamina battles (L 1)"},
        {160, "Strong-Wind Resist", 1, true, "Reduces pushback from wind hazards (L 1)"},
        {161, "Strong-Wind Immunity", 2, true, "Completely negates wind hazards (L 2)"},
        {162, "Critical-Health Healing", 2, true, "Heals 50% damage once when reaching critical percent (L 2)"},
        {163, "Special-Move Power Up", 1, true, "Increases damage of all four special moves (L 1)"},
        {164, "Reserved_164", 1, false, "Unused internal slot 164"},
        {165, "Bob-omb Equipped", 1, true, "Starts match holding a Bob-omb (L 1)"},
        {166, "Hothead Equipped", 2, true, "Starts match holding a Hothead (L 2)"},
        {167, "Super Leaf Equipped", 1, true, "Starts match wearing a Super Leaf (L 1)"},
        {168, "Super Launch Star Equipped", 1, true, "Starts match holding a Super Launch Star (L 1)"},
        {169, "Beastball Equipped", 1, true, "Starts match holding a Beastball (L 1)"},
        {170, "Death's Scythe Equipped", 1, true, "Starts match holding Death's Scythe (L 1)"},
        {171, "Mr. Saturn Equipped", 1, true, "Starts match holding Mr. Saturn (L 1)"},
        {172, "Unira Equipped", 1, true, "Starts match holding an Unira (L 1)"},
        {173, "Rocket Belt Equipped", 1, true, "Starts match wearing a Rocket Belt (L 1)"},
        {174, "Black Hole Equipped", 2, true, "Starts match holding a Black Hole (L 2)"},
        {175, "Reserved_175", 1, false, "Unused internal slot 175"},
        {176, "Stats Up Up after Eating", 2, true, "Greatly boosts all stats after eating food (L 2)"},
        {177, "Reserved_177", 1, false, "Unused internal slot 177"},
        {178, "Critical-Health Stats Up Up", 2, true, "Greatly boosts attack, defense, and speed at high percent (L 2)"},
        {179, "Critical Hit Up Up", 2, true, "Grants a 12% chance to deal 2x damage (L 2)"},
        {180, "Great Autoheal", 3, true, "Recovers 5% damage every 3 seconds (L 3)"},
        {181, "Steel Diver Equipped", 2, true, "Starts match holding a Steel Diver (L 2)"},
        {182, "Banana Gun Equipped", 1, true, "Starts match holding a Banana Gun (L 1)"},
        {183, "Rage Blaster Equipped", 1, true, "Starts match holding a Rage Blaster (L 1)"},
        {184, "Staff Equipped", 1, true, "Starts match holding a Staff (L 1)"},
        {185, "Fire Bar Equipped", 1, true, "Starts match holding a Fire Bar (L 1)"},
        {186, "Screw Attack Equipped", 2, true, "Starts match wearing a Screw Attack badge (L 2)"},
        {187, "Bomber Equipped", 1, true, "Starts match holding a Bomber (L 1)"},
        {188, "Cucco Equipped", 1, false, "Starts match holding a Cucco"},
        {189, "Neutral Attack Up", 1, false, "Increases neutral jab attack damage"},
        {190, "Neutral Attack Up Up", 2, false, "Greatly increases neutral jab attack damage"},
        {191, "Tilt Attack Up", 1, false, "Increases tilt attack damage"},
        {192, "Tilt Attack Up Up", 2, false, "Greatly increases tilt attack damage"},
        {193, "Air Attack Up Up", 2, false, "Greatly increases aerial attack damage"},
        {194, "Mighty Throw", 2, false, "Greatly increases throw damage and knockback"},
        {195, "Special-Move Power Up Up", 2, false, "Greatly increases special move damage"},
        {196, "Super Easy Dodging", 2, false, "Greatly increases dodge invincibility window"},
        {197, "Reserved_197", 1, false, "Unused internal slot 197"},
        {198, "Landing Lag Down Down", 2, false, "Greatly reduces landing lag on aerials"},
        {199, "Become Heavy", 1, false, "Increases fighter weight and knockback resistance"},
        {200, "Meteor Smashes Up", 1, false, "Increases meteor smash knockback speed"},
        {201, "Poisoned Smash", 2, false, "Smash attacks inflict poison damage"},
        {202, "No Penalty for Continuous Dodging", 2, false, "Removes stale-dodge frame penalty"},
        {203, "Airborne Endurance", 1, false, "Reduces knockback taken while airborne"},
        {204, "Sprinting Endurance", 1, false, "Reduces knockback taken while dashing"},
        {205, "Perfect-Shield Recovery", 2, false, "Heals fighter when landing a parry"},
        {206, "Masterful Fall Break", 1, false, "Automatic teching when hitting stage surfaces"},
        {207, "Healing-Item Attraction", 1, false, "Draws healing items toward fighter"},
        {208, "Attack Up When Healthy", 1, false, "Boosts attack while at low damage"},
        {209, "Defense Up When Healthy", 1, false, "Boosts defense while at low damage"},
        {210, "Endless Smash Holding", 2, false, "Allows holding smash attacks indefinitely"},
        {211, "Heal with Smash Attacks", 2, false, "Heals damage when landing smash attacks"},
        {212, "Activities Up", 1, false, "Boosts general action frequency"},
        {213, "Giant Killer", 1, true, "Deals extra damage against Giant opponents (L 1)"},
        {214, "Metal Killer", 1, true, "Deals extra damage against Metal opponents (L 1)"},
        {215, "Assist Killer", 1, true, "Deals extra damage against Assist Trophies (L 1)"},
        {216, "Jam FS Charge", 2, true, "Slows opponent Final Smash meter charge (L 2)"},
        {217, "Weapon Resist Up", 2, true, "Reduces damage taken from weapon attacks (L 2)"},
        {218, "Hyper Smash Attacks Bonus", 1, false, "Secondary Hyper Smash Attacks ID"},
        {219, "Neutral Attack Up Bonus", 1, false, "Secondary Neutral Attack Up ID"},
        {220, "Tilt Attack Up Bonus", 1, false, "Secondary Tilt Attack Up ID"},
        {221, "Special-Move Power Up Bonus", 1, false, "Secondary Special-Move Power Up ID"},
        {222, "Air Attack Up Bonus", 1, false, "Secondary Air Attack Up ID"},
        {223, "Reserved_223", 1, false, "Unused internal slot 223"},
        {224, "Reserved_224", 1, false, "Unused internal slot 224"},
        {225, "Reserved_225", 1, false, "Unused internal slot 225"},
        {226, "Reserved_226", 1, false, "Unused internal slot 226"},
        {227, "Item Autograb", 1, true, "Automatically picks up nearby items (L 1)"},
        {228, "Team Power Up", 2, true, "Boosts team attack and defense in team battles (L 2)"},
        {229, "Weight Up Up", 2, false, "Greatly increases weight and knockback resistance"},
        {230, "Poison Power Up", 3, true, "Boosts attack power while poisoned (L 3)"},
        {231, "Critical Fast Final Smash Meter Up Up", 3, true, "Rapidly charges Final Smash meter at critical health (L 3)"},
        {232, "Critical-Health Healing Up Up", 3, true, "Heals 100% damage once when reaching critical health (L 3)"},
        {233, "Critical Super Giant", 3, true, "Becomes Giant when reaching critical health (L 3)"},
        {234, "Mouthful of Curry Up Up", 3, true, "Extended Superspicy Curry duration (L 3)"},
        {235, "Final Smash Up", 2, true, "Increases Final Smash damage and knockback (L 2)"},
        {236, "Critical Healing & Metal", 2, true, "Heals and turns Metal at critical health (L 2)"},
        {237, "Fist & Foot Attack Up", 1, false, "Increases both punch and kick attack damage"},
        {238, "Reserved_238", 1, false, "Unused internal slot 238"},
        {239, "Reserved_239", 1, false, "Unused internal slot 239"},
        {240, "Reserved_240", 1, false, "Unused internal slot 240"},
        {241, "Reserved_241", 1, false, "Unused internal slot 241"},
        {242, "Reserved_242", 1, false, "Unused internal slot 242"},
        {243, "Reserved_243", 1, false, "Unused internal slot 243"},
        {244, "Reserved_244", 1, false, "Unused internal slot 244"},
        {245, "Reserved_245", 1, false, "Unused internal slot 245"},
        {246, "Reserved_246", 1, false, "Unused internal slot 246"},
        {247, "Reserved_247", 1, false, "Unused internal slot 247"},
        {248, "Reserved_248", 1, false, "Unused internal slot 248"},
        {249, "Reserved_249", 1, false, "Unused internal slot 249"},
        {250, "Reserved_250", 1, false, "Unused internal slot 250"},
        {251, "Reserved_251", 1, false, "Unused internal slot 251"},
        {252, "Reserved_252", 1, false, "Unused internal slot 252"},
        {253, "Reserved_253", 1, false, "Unused internal slot 253"},
        {254, "Reserved_254", 1, false, "Unused internal slot 254"}
    };
    return kCatalog;
}

const SupportSpiritAbility* FindSupportAbilityById(uint8_t id) {
    const auto& cat = GetSupportSpiritCatalog();
    if (id < cat.size()) {
        return &cat[id];
    }
    return nullptr;
}

const SupportSpiritAbility* FindSupportAbilityByName(const std::string& query) {
    std::string q = ToLower(query);
    const auto& cat = GetSupportSpiritCatalog();
    for (const auto& s : cat) {
        if (ToLower(s.name) == q) return &s;
    }
    for (const auto& s : cat) {
        if (ToLower(s.name).find(q) != std::string::npos) return &s;
    }
    return nullptr;
}

// Curated Primary Spirits from Super Smash Bros. Ultimate Spirit Collection
const std::vector<PrimarySpiritPreset>& GetPrimarySpiritCatalog() {
    static const std::vector<PrimarySpiritPreset> kPrimary = {
        {0,    "No Spirit (Vanilla 0/0)", SpiritType::Neutral, 0,    0,    3, "Super Smash Bros."},
        {21,   "Metal Mario",             SpiritType::Defense, 1800, 3200, 1, "Super Mario"},
        {48,   "Bowser (Wedding)",        SpiritType::Attack,  2800, 2200, 3, "Super Mario"},
        {130,  "Daruk",                   SpiritType::Defense, 2100, 2900, 2, "The Legend of Zelda"},
        {142,  "Calamity Ganon",          SpiritType::Attack,  3100, 1900, 1, "The Legend of Zelda"},
        {205,  "Queen Metroid",           SpiritType::Grab,    2650, 2350, 3, "Metroid"},
        {312,  "Gyarados",                SpiritType::Attack,  2700, 2300, 2, "Pokemon"},
        {498,  "Zacian & Zamazenta",      SpiritType::Neutral, 2500, 2500, 3, "Pokemon"},
        {612,  "Black Knight",            SpiritType::Defense, 2400, 2600, 2, "Fire Emblem"},
        {631,  "Sirius",                  SpiritType::Attack,  2750, 2250, 3, "Fire Emblem"},
        {750,  "Super Sonic",             SpiritType::Grab,    2600, 2400, 3, "Sonic the Hedgehog"},
        {854,  "Chibi-Robo (Super)",      SpiritType::Grab,    2600, 2100, 2, "Chibi-Robo!"},
        {988,  "Akuma",                   SpiritType::Attack,  3400, 1600, 0, "Street Fighter"},
        {1052, "Soma Cruz",               SpiritType::Neutral, 2550, 2450, 3, "Castlevania"},
        {1112, "Big Boss",                SpiritType::Defense, 2300, 2700, 3, "Metal Gear"},
        {1201, "Toad",                    SpiritType::Grab,    2100, 2600, 3, "Super Mario"},
        {1240, "Digby",                   SpiritType::Neutral, 2400, 2200, 3, "Animal Crossing"},
        {1255, "DJ K.K.",                 SpiritType::Neutral, 2150, 2850, 3, "Animal Crossing"},
        {1297, "Galeem",                  SpiritType::Neutral, 2500, 2500, 3, "Super Smash Bros."},
        {1298, "Dharkon",                 SpiritType::Neutral, 2500, 2500, 3, "Super Smash Bros."},
        {1302, "Tabuu",                   SpiritType::Neutral, 2650, 2350, 3, "Super Smash Bros."},
        {1465, "Sephiroth (One-Winged)",  SpiritType::Attack,  2950, 2050, 3, "Final Fantasy"},
        {1480, "Mythra (Awakened)",       SpiritType::Grab,    2700, 2300, 3, "Xenoblade Chronicles"}
    };
    return kPrimary;
}

const PrimarySpiritPreset* FindPrimarySpiritByName(const std::string& query) {
    std::string q = ToLower(query);
    const auto& cat = GetPrimarySpiritCatalog();
    for (const auto& p : cat) {
        if (ToLower(p.name) == q) return &p;
    }
    for (const auto& p : cat) {
        if (ToLower(p.name).find(q) != std::string::npos) return &p;
    }
    return nullptr;
}

// Representative SSBU Fighter Roster with NTAG215 16-bit Character IDs and combat archetypes
const std::vector<FighterInfo>& GetFighterCatalog() {
    static const std::vector<FighterInfo> kFighters = {
        {0x0000, "Mario",            1.00f, 1.05f, 0.35f, 1.15f},
        {0x0001, "Luigi",            0.98f, 0.85f, 0.30f, 1.45f},
        {0x0002, "Peach",            0.90f, 1.25f, 0.40f, 1.10f},
        {0x0003, "Daisy",            0.90f, 1.25f, 0.40f, 1.10f},
        {0x0005, "Bowser",           1.40f, 0.95f, 0.25f, 1.35f},
        {0x0008, "Donkey Kong",      1.32f, 1.00f, 0.05f, 1.40f},
        {0x0009, "Diddy Kong",       0.92f, 1.15f, 0.55f, 1.25f},
        {0x0100, "Link",             1.06f, 0.90f, 0.75f, 1.00f},
        {0x0101, "Zelda",            0.86f, 0.90f, 0.70f, 1.05f},
        {0x0102, "Ganondorf",        1.25f, 0.75f, 0.00f, 1.15f},
        {0x0104, "Young Link",       0.88f, 1.10f, 0.85f, 1.05f},
        {0x0500, "Samus",            1.10f, 0.95f, 0.85f, 1.20f},
        {0x0501, "Dark Samus",       1.10f, 0.95f, 0.85f, 1.20f},
        {0x0503, "Ridley",           1.09f, 1.10f, 0.35f, 1.20f},
        {0x1F00, "Kirby",            0.78f, 1.10f, 0.15f, 1.05f},
        {0x1F01, "King Dedede",      1.35f, 0.80f, 0.45f, 1.15f},
        {0x1F02, "Meta Knight",      0.80f, 1.25f, 0.05f, 1.00f},
        {0x0580, "Fox",              0.77f, 1.35f, 0.30f, 1.00f},
        {0x0581, "Falco",            0.82f, 1.25f, 0.40f, 1.15f},
        {0x0582, "Wolf",             0.94f, 1.20f, 0.50f, 1.15f},
        {0x1919, "Pikachu",          0.79f, 1.30f, 0.45f, 1.15f},
        {0x1996, "Jigglypuff",       0.68f, 1.45f, 0.00f, 0.90f},
        {0x199A, "Mewtwo",           0.79f, 1.25f, 0.55f, 1.20f},
        {0x1A04, "Lucario",          0.94f, 1.15f, 0.50f, 1.10f},
        {0x1C13, "Incineroar",       1.22f, 0.80f, 0.10f, 1.45f},
        {0x0600, "Captain Falcon",   1.06f, 1.25f, 0.00f, 1.20f},
        {0x2280, "Ness",             0.94f, 1.15f, 0.60f, 1.30f},
        {0x2100, "Marth",            0.90f, 1.15f, 0.05f, 1.05f},
        {0x2101, "Lucina",           0.90f, 1.15f, 0.05f, 1.05f},
        {0x2102, "Roy",              0.96f, 1.25f, 0.05f, 1.15f},
        {0x2103, "Ike",              1.12f, 1.05f, 0.10f, 1.15f},
        {0x2106, "Byleth",           0.98f, 0.95f, 0.40f, 1.15f},
        {0x0780, "King K. Rool",     1.38f, 0.85f, 0.65f, 1.25f},
        {0x3200, "Sonic",            0.86f, 1.35f, 0.20f, 1.05f},
        {0x3480, "Mega Man",         1.04f, 1.00f, 0.90f, 1.05f},
        {0x3340, "Pac-Man",          0.95f, 1.15f, 0.80f, 1.15f},
        {0x34C0, "Ryu",              1.05f, 0.95f, 0.45f, 1.15f},
        {0x34C1, "Ken",              1.05f, 1.00f, 0.35f, 1.15f},
        {0x3600, "Cloud",            1.02f, 1.20f, 0.35f, 1.00f},
        {0x3601, "Sephiroth",        0.79f, 1.15f, 0.55f, 1.10f},
        {0x3680, "Bayonetta",        0.81f, 1.25f, 0.30f, 1.00f},
        {0x3740, "Joker",            0.93f, 1.25f, 0.45f, 1.15f},
        {0x3800, "Terry",            1.08f, 1.05f, 0.35f, 1.15f},
        {0x38C0, "Steve",            0.92f, 1.10f, 0.65f, 1.20f},
        {0x3900, "Kazuya",           1.15f, 0.90f, 0.30f, 1.40f},
        {0x3940, "Sora",             0.85f, 1.35f, 0.45f, 1.05f}
    };
    return kFighters;
}

const FighterInfo* FindFighterById(uint16_t char_id) {
    const auto& cat = GetFighterCatalog();
    for (const auto& f : cat) {
        if (f.char_id == char_id) return &f;
    }
    return &cat[0];
}

const FighterInfo* FindFighterByName(const std::string& query) {
    std::string q = ToLower(query);
    const auto& cat = GetFighterCatalog();
    for (const auto& f : cat) {
        if (ToLower(f.name) == q) return &f;
    }
    for (const auto& f : cat) {
        if (ToLower(f.name).find(q) != std::string::npos) return &f;
    }
    return &cat[0];
}

} // namespace ssbu
