#include "ssbu_loadouts.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>

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
        case SpiritType::Attack:  return "Attack (Red)";
        case SpiritType::Defense: return "Defense (Blue)";
        case SpiritType::Grab:    return "Grab (Green)";
        default:                  return "Neutral";
    }
}

// Complete 255-entry SSBU Support Spirit Ability table (0xEC, 0xED, 0xEE)
// Grounded in jozz024/smash-amiibo-editor/resources/abilities.txt and MiDe's SSBU Amiibo Bin Guide
const std::vector<SupportSpiritAbility>& GetSupportSpiritCatalog() {
    static const std::vector<SupportSpiritAbility> kCatalog = []() {
        std::vector<SupportSpiritAbility> v(255);
        for (int i = 0; i < 255; ++i) {
            v[i] = {static_cast<uint8_t>(i), "Reserved", 1, false, false, "Unused internal ability ID"};
        }
        auto set = [&](uint8_t id, const char* name, uint8_t cost, bool legal, const char* desc) {
            v[id] = {id, name, cost, legal, true, desc};
        };

        set(0,   "None", 0, true, "Empty spirit slot");
        set(1,   "Move Speed Up", 1, true, "Increases ground movement speed (L 1)");
        set(2,   "Hyper Smash Attacks", 1, true, "Increases smash attack charge speed and damage (L 1)");
        set(4,   "Jump Up", 1, true, "Increases jump height (L 1)");
        set(5,   "Additional Midair Jump", 2, true, "Grants an extra midair jump (L 2)");
        set(6,   "Lifesteal", 3, false, "Heals when dealing damage to opponents (U 3)");
        set(7,   "Defense Up", 1, false, "Increases defense stat multiplier");
        set(8,   "Easier Dodging", 1, true, "Increases dodge invincibility window (L 1)");
        set(9,   "Easier Perfect Shield", 1, true, "Widens parry timing window (L 1)");
        set(10,  "Super Armor", 3, true, "Prevents flinching from weak and medium attacks (L 3)");
        set(11,  "Slow Super Armor", 2, true, "Grants Super Armor at reduced movement speed (L 2)");
        set(12,  "Trade-Off Attacks Up", 1, true, "Starts match with 30% damage and higher attack power (L 1)");
        set(13,  "Trade-Off Defense Up", 1, true, "Starts match with 30% damage and higher defense (L 1)");
        set(14,  "Trade-Off Speed Up", 1, true, "Starts match with 30% damage and higher speed (L 1)");
        set(15,  "Trade-Off Ability Up", 1, true, "Starts match with 30% damage and boosted stats (L 1)");
        set(16,  "Critical-Health Attack Up", 1, true, "Boosts attack power at high damage percent (L 1)");
        set(17,  "Critical-Health Defense Up", 1, true, "Boosts defense at high damage percent (L 1)");
        set(18,  "Critical-Health Stats Up", 1, true, "Boosts attack, defense, and speed at high percent (L 1)");
        set(19,  "Critical Immunity", 2, true, "Grants brief invincibility when reaching critical health (L 2)");
        set(20,  "Autoheal", 2, true, "Recovers 3% damage every 5 seconds (L 2)");
        set(21,  "Poison Immunity", 2, true, "Negates poison floor hazards (L 2)");
        set(22,  "Poison Damage Reduced", 1, true, "Reduces damage taken from poison hazards (L 1)");
        set(23,  "Poison Heals", 3, true, "Reverses poison damage into healing (L 3)");
        set(24,  "Lava-Floor Immunity", 2, true, "Negates lava floor hazards (L 2)");
        set(25,  "Sticky-Floor Immunity", 2, true, "Negates sticky floor hazards (L 2)");
        set(26,  "Beam Sword Equipped", 1, true, "Starts match holding a Beam Sword (L 1)");
        set(27,  "Lip's Stick Equipped", 1, true, "Starts match holding Lip's Stick (L 1)");
        set(28,  "Star Rod Equipped", 1, true, "Starts match holding a Star Rod (L 1)");
        set(29,  "Ore Club Equipped", 3, true, "Starts match holding an Ore Club (L 3)");
        set(30,  "Home-Run Bat Equipped", 3, false, "Starts match holding a Home-Run Bat");
        set(31,  "Ray Gun Equipped", 2, true, "Starts match holding a Ray Gun (L 2)");
        set(32,  "Super Scope Equipped", 2, true, "Starts match holding a Super Scope (L 2)");
        set(33,  "Gust Bellows Equipped", 2, false, "Starts match holding a Gust Bellows");
        set(34,  "Drill Equipped", 1, true, "Starts match holding a Drill (L 1)");
        set(35,  "Green Shell Equipped", 1, true, "Starts match holding a Green Shell (L 1)");
        set(36,  "Poke Ball Equipped", 2, false, "Starts match holding a Poke Ball");
        set(38,  "Back Shield Equipped", 1, true, "Starts match with a Back Shield (L 1)");
        set(39,  "Bunny Hood Equipped", 1, true, "Starts match wearing a Bunny Hood (L 1)");
        set(40,  "Made of Metal", 1, true, "Starts match in Metal form (L 1)");
        set(41,  "Mouthful of Curry", 2, true, "Starts match with Superspicy Curry active (L 2)");
        set(42,  "Franklin Badge Equipped", 2, true, "Starts match wearing a Franklin Badge to reflect projectiles (L 2)");
        set(43,  "Hammer Equipped", 3, false, "Starts match holding a Hammer");
        set(44,  "Fairy Bottle Equipped", 1, true, "Starts match holding a Fairy Bottle (L 1)");
        set(45,  "Fire Flower Equipped", 1, true, "Starts match holding a Fire Flower (L 1)");
        set(46,  "Freezie Equipped", 1, true, "Starts match holding a Freezie (L 1)");
        set(47,  "Ramblin' Evil Mushroom Equipped", 1, true, "Starts match holding a Ramblin' Evil Mushroom (L 1)");
        set(48,  "Killing Edge Equipped", 1, true, "Starts match holding a Killing Edge (L 1)");
        set(50,  "Physical Attack Up", 1, true, "Increases punch, kick, and bite damage by 1.1x (L 1)");
        set(51,  "Weapon Attack Up", 1, true, "Increases sword, hammer, and weapon damage by 1.1x (L 1)");
        set(52,  "Fist Attack Up", 1, true, "Increases punch and elbow attack damage by 1.1x (L 1)");
        set(53,  "Foot Attack Up", 1, true, "Increases kick and knee attack damage by 1.1x (L 1)");
        set(54,  "Aura Attack Up", 1, true, "Increases Aura attack damage (L 1)");
        set(55,  "Magic Attack Up", 1, true, "Increases magic attack damage (L 1)");
        set(56,  "PSI Attack Up", 1, true, "Increases PSI attack damage (L 1)");
        set(58,  "Fire & Explosion Attack Up", 1, true, "Increases fire and explosion damage (L 1)");
        set(60,  "Electric Attack Up", 1, true, "Increases electric attack damage (L 1)");
        set(61,  "Energy-Shot Attack Up", 1, true, "Increases energy projectile damage (L 1)");
        set(62,  "Water & Ice Attack Up", 1, true, "Increases water and freezing attack damage (L 1)");
        set(63,  "Magic Resist Up", 1, true, "Reduces damage taken from magic attacks (L 1)");
        set(64,  "PSI Resist Up", 1, true, "Reduces damage taken from PSI attacks (L 1)");
        set(66,  "Fire/Explosion Resist Up", 1, true, "Reduces damage taken from fire and explosions (L 1)");
        set(69,  "Electric Resist Up", 1, true, "Reduces damage taken from electric attacks (L 1)");
        set(70,  "Energy-Shot Resist Up", 1, true, "Reduces damage taken from energy projectiles (L 1)");
        set(72,  "Water/Freezing Resist Up", 1, true, "Reduces damage taken from water and ice attacks (L 1)");
        set(73,  "Aura Resist Up", 1, true, "Reduces damage taken from Aura attacks (L 1)");
        set(74,  "Zap-Floor Immunity", 2, true, "Negates electrified floor hazards (L 2)");
        set(75,  "Slumber Immunity", 1, true, "Prevents sleep status effects (L 1)");
        set(76,  "Ice-Floor Immunity", 2, true, "Negates icy floor slipping and freezing (L 2)");
        set(77,  "Falling Immunity", 1, true, "Prevents tripping (L 1)");
        set(78,  "Bury Immunity", 1, true, "Prevents being buried in the ground (L 1)");
        set(79,  "Braking Ability Up", 1, true, "Increases ground traction and stopping speed (L 1)");
        set(80,  "Mobility Up", 1, false, "Boosts overall air and ground mobility");
        set(81,  "Landing Lag Down", 1, true, "Reduces landing lag on aerial attacks (L 1)");
        set(82,  "Lightweight", 1, true, "Increases mobility and jump height while reducing weight (L 1)");
        set(83,  "Shield Damage Up", 1, true, "Deals extra damage to opponent shields (L 1)");
        set(84,  "Air Attack Up", 1, true, "Increases aerial attack damage (L 1)");
        set(85,  "Air Defense Up", 1, true, "Reduces damage taken while airborne (L 1)");
        set(86,  "Neutral Special Up", 1, true, "Increases neutral special damage (L 1)");
        set(87,  "Side Special Up", 1, true, "Increases side special damage (L 1)");
        set(88,  "Up Special Up", 1, true, "Increases up special damage (L 1)");
        set(89,  "Down Special Up", 1, true, "Increases down special damage (L 1)");
        set(90,  "Strong Throw", 1, true, "Increases throw damage and knockback (L 1)");
        set(91,  "Unflinching Charged Smashes", 2, true, "Grants super armor while charging smash attacks (L 2)");
        set(92,  "Toss & Meteor", 1, true, "Increases up-throw and meteor smash power (L 1)");
        set(94,  "Critical Hit Up", 1, true, "Grants a 5% chance to deal 2x damage (L 1)");
        set(95,  "Swimmer", 1, true, "Allows unlimited swimming in water (L 1)");
        set(96,  "Shield Durability Up", 1, true, "Increases maximum shield health (L 1)");
        set(97,  "Improved Escape", 1, true, "Makes mashing out of grabs, buries, and stuns faster (L 1)");
        set(101, "Battering Items Up", 1, true, "Increases damage dealt with battering items (L 1)");
        set(102, "Shooting Items Up", 1, true, "Increases ammo and power of shooting items (L 1)");
        set(103, "Thrown Items Up", 1, true, "Increases speed and power of thrown items (L 1)");
        set(104, "KOs Heal Damage", 2, true, "Heals damage whenever scoring a KO (L 2)");
        set(105, "Invincibility after Eating", 2, true, "Grants brief invincibility after eating food (L 2)");
        set(106, "Stats Up after Eating", 1, true, "Boosts attack, defense, and speed after eating (L 1)");
        set(108, "First-Strike Advantage", 2, true, "Grants brief invincibility when landing the first hit of a match (L 2)");
        set(110, "Running Start", 2, true, "Boosts movement speed at the start of a match (L 2)");
        set(112, "Fast Final Smash Meter", 2, true, "Charges Final Smash meter faster (L 2)");
        set(113, "Instadrop", 2, true, "Enables damaging fast-fall drop (L 2)");
        set(114, "Healing Shield", 2, true, "Heals fighter when shielding attacks (L 2)");
        set(117, "Floaty Jumps", 1, true, "Reduces gravity while jumping and falling (L 1)");
        set(119, "Irreversible Controls", 2, true, "Prevents control-reversal status effects (L 2)");
        set(120, "Recovery Items Up", 1, false, "Increases healing received from recovery items");
        set(121, "Transformation Duration Up", 1, true, "Extends duration of fighter transformations and power-ups (L 1)");
        set(122, "Undamaged Attack Up", 1, true, "Boosts attack power while at 0% damage (L 1)");
        set(123, "Undamaged Speed Up", 1, true, "Boosts movement speed while at 0% damage (L 1)");
        set(124, "Undamaged Attack & Speed Up", 1, true, "Boosts attack and speed while at 0% damage (L 1)");
        set(127, "Edge Grab Up", 1, false, "Increases ledge snap range");
        set(128, "Impact Run", 1, true, "Deals damage when dashing into opponents (L 1)");
        set(130, "Lava-Floor Resist", 1, true, "Reduces damage taken from lava floors (L 1)");
        set(131, "Item Gravitation", 1, true, "Pulls nearby items toward the fighter (L 1)");
        set(133, "Chance of Double Final Smash", 2, true, "Chance to use Final Smash twice in a row (L 2)");
        set(134, "Double Final Smash", 3, true, "Guarantees a second Final Smash after the first (L 3)");
        set(137, "Metal and Giant", 3, false, "Starts match both Metal and Giant");
        set(138, "Giant", 2, true, "Starts match as a Giant fighter (L 2)");
        set(139, "Dash Attack Up", 1, true, "Increases dash attack damage and speed (L 1)");
        set(140, "Armor Knight", 2, true, "Greatly boosts defense and attack while reducing speed (L 2)");
        set(142, "Energy Shot Attack/Resistance Up", 2, true, "Boosts energy projectile damage and resistance (L 2)");
        set(143, "Hammer Duration Up", 1, true, "Extends duration of Hammer and Golden Hammer items (L 1)");
        set(144, "Boomerang Equipped", 1, true, "Starts match holding a Boomerang (L 1)");
        set(145, "Item Attack Up", 1, false, "Boosts damage of all item attacks");
        set(151, "Perfect-Shield Reflect", 1, true, "Parrying reflects projectiles with boosted speed (L 1)");
        set(152, "Weapon Attack & Move Speed Up", 2, true, "Boosts weapon attack damage and movement speed (L 2)");
        set(153, "Shooting Attack Up", 1, true, "Boosts fighter projectile and shooting attacks (L 1)");
        set(154, "Charge Speed & Power Up", 2, false, "Increases charge speed and power of chargeable moves");
        set(156, "Screen-Flip Immunity", 2, true, "Negates screen-flip stage hazards (L 2)");
        set(157, "Fog Immunity", 2, true, "Clears fog stage hazards (L 2)");
        set(158, "Gravity-Change Immunity", 2, true, "Negates gravity-change stage hazards (L 2)");
        set(159, "Stamina Up", 1, true, "Increases starting HP in Stamina battles (L 1)");
        set(160, "Strong-Wind Resist", 1, true, "Reduces pushback from wind hazards (L 1)");
        set(161, "Strong-Wind Immunity", 2, true, "Completely negates wind hazards (L 2)");
        set(162, "Critical-Health Healing", 2, true, "Heals 50% damage once when reaching critical percent (L 2)");
        set(163, "Special-Move Power Up", 1, true, "Increases damage of all four special moves (L 1)");
        set(165, "Bob-omb Equipped", 1, true, "Starts match holding a Bob-omb (L 1)");
        set(166, "Hothead Equipped", 2, true, "Starts match holding a Hothead (L 2)");
        set(167, "Super Leaf Equipped", 1, true, "Starts match wearing a Super Leaf (L 1)");
        set(168, "Super Launch Star Equipped", 1, true, "Starts match holding a Super Launch Star (L 1)");
        set(169, "Beastball Equipped", 1, true, "Starts match holding a Beastball (L 1)");
        set(170, "Death's Scythe Equipped", 1, true, "Starts match holding Death's Scythe (L 1)");
        set(171, "Mr. Saturn Equipped", 1, true, "Starts match holding Mr. Saturn (L 1)");
        set(172, "Unira Equipped", 1, true, "Starts match holding an Unira (L 1)");
        set(173, "Rocket Belt Equipped", 1, true, "Starts match wearing a Rocket Belt (L 1)");
        set(174, "Black Hole Equipped", 2, true, "Starts match holding a Black Hole (L 2)");
        set(176, "Stats Up Up after Eating", 2, true, "Greatly boosts all stats after eating food (L 2)");
        set(178, "Critical-Health Stats Up Up", 2, true, "Greatly boosts attack, defense, and speed at high percent (L 2)");
        set(179, "Critical Hit Up Up", 2, true, "Grants a 12% chance to deal 2x damage (L 2)");
        set(180, "Great Autoheal", 3, true, "Recovers 5% damage every 3 seconds (L 3)");
        set(181, "Steel Diver Equipped", 2, true, "Starts match holding a Steel Diver (L 2)");
        set(182, "Banana Gun Equipped", 1, true, "Starts match holding a Banana Gun (L 1)");
        set(183, "Rage Blaster Equipped", 1, true, "Starts match holding a Rage Blaster (L 1)");
        set(184, "Staff Equipped", 1, true, "Starts match holding a Staff (L 1)");
        set(185, "Fire Bar Equipped", 1, true, "Starts match holding a Fire Bar (L 1)");
        set(186, "Screw Attack Equipped", 2, true, "Starts match wearing a Screw Attack badge (L 2)");
        set(187, "Bomber Equipped", 1, true, "Starts match holding a Bomber (L 1)");
        set(188, "Cucco Equipped", 1, false, "Starts match holding a Cucco");
        set(189, "Neutral Attack Up", 1, false, "Increases neutral jab attack damage");
        set(190, "Neutral Attack Up Up", 2, false, "Greatly increases neutral jab attack damage");
        set(191, "Tilt Attack Up", 1, false, "Increases tilt attack damage");
        set(192, "Tilt Attack Up Up", 2, false, "Greatly increases tilt attack damage");
        set(193, "Air Attack Up Up", 2, false, "Greatly increases aerial attack damage");
        set(194, "Mighty Throw", 2, false, "Greatly increases throw damage and knockback");
        set(195, "Special-Move Power Up Up", 2, false, "Greatly increases special move damage");
        set(196, "Super Easy Dodging", 2, false, "Greatly increases dodge invincibility window");
        set(198, "Landing Lag Down Down", 2, false, "Greatly reduces landing lag on aerials");
        set(199, "Become Heavy", 1, false, "Increases fighter weight and knockback resistance");
        set(200, "Meteor Smashes Up", 1, false, "Increases meteor smash knockback speed");
        set(201, "Poisoned Smash", 2, false, "Smash attacks inflict poison damage");
        set(202, "No Penalty for Continuous Dodging", 2, false, "Removes stale-dodge frame penalty");
        set(203, "Airborne Endurance", 1, false, "Reduces knockback taken while airborne");
        set(204, "Sprinting Endurance", 1, false, "Reduces knockback taken while dashing");
        set(205, "Perfect-Shield Recovery", 2, false, "Heals fighter when landing a parry");
        set(206, "Masterful Fall Break", 1, false, "Automatic teching when hitting stage surfaces");
        set(207, "Healing-Item Attraction", 1, false, "Draws healing items toward fighter");
        set(208, "Attack Up When Healthy", 1, false, "Boosts attack while at low damage");
        set(209, "Defense Up When Healthy", 1, false, "Boosts defense while at low damage");
        set(210, "Endless Smash Holding", 2, false, "Allows holding smash attacks indefinitely");
        set(211, "Heal with Smash Attacks", 2, false, "Heals damage when landing smash attacks");
        set(212, "Activities Up", 1, false, "Boosts general action frequency");
        set(213, "Giant Killer", 1, true, "Deals extra damage against Giant opponents (L 1)");
        set(214, "Metal Killer", 1, true, "Deals extra damage against Metal opponents (L 1)");
        set(215, "Assist Killer", 1, true, "Deals extra damage against Assist Trophies (L 1)");
        set(216, "Jam FS Charge", 2, true, "Slows opponent Final Smash meter charge (L 2)");
        set(217, "Weapon Resist Up", 2, true, "Reduces damage taken from weapon attacks (L 2)");
        set(227, "Item Autograb", 1, true, "Automatically picks up nearby items (L 1)");
        set(228, "Team Power Up", 2, true, "Boosts team attack and defense in team battles (L 2)");
        set(229, "Weight Up Up", 2, false, "Greatly increases weight and knockback resistance");
        set(230, "Poison Power Up", 3, true, "Boosts attack power while poisoned (L 3)");
        set(231, "Critical Fast Final Smash Meter Up Up", 3, true, "Rapidly charges Final Smash meter at critical health (L 3)");
        set(232, "Critical-Health Healing Up Up", 3, true, "Heals 100% damage once when reaching critical health (L 3)");
        set(233, "Critical Super Giant", 3, true, "Becomes Giant when reaching critical health (L 3)");
        set(234, "Mouthful of Curry Up Up", 3, true, "Extended Superspicy Curry duration (L 3)");
        set(235, "Final Smash Up", 2, true, "Increases Final Smash damage and knockback (L 2)");
        set(236, "Critical Healing & Metal", 2, true, "Heals and turns Metal at critical health (L 2)");
        set(237, "Fist & Foot Attack Up", 1, false, "Increases both punch and kick attack damage");
        return v;
    }();
    return kCatalog;
}

std::vector<const SupportSpiritAbility*> GetSelectableSupportSpirits(bool legal_only, const std::string& filter) {
    std::vector<const SupportSpiritAbility*> out;
    std::string q = ToLower(filter);
    for (const auto& s : GetSupportSpiritCatalog()) {
        if (!s.is_selectable) continue;
        if (legal_only && !s.tournament_legal) continue;
        if (!q.empty() && s.id != 0) {
            if (ToLower(s.name).find(q) == std::string::npos &&
                ToLower(s.description).find(q) == std::string::npos) {
                continue;
            }
        }
        out.push_back(&s);
    }
    return out;
}

const SupportSpiritAbility* FindSupportAbilityById(uint8_t id) {
    const auto& cat = GetSupportSpiritCatalog();
    if (id < cat.size()) return &cat[id];
    return nullptr;
}

const SupportSpiritAbility* FindSupportAbilityByName(const std::string& query) {
    std::string q = ToLower(query);
    const auto& cat = GetSupportSpiritCatalog();
    for (const auto& s : cat) {
        if (s.is_selectable && ToLower(s.name) == q) return &s;
    }
    for (const auto& s : cat) {
        if (s.is_selectable && ToLower(s.name).find(q) != std::string::npos) return &s;
    }
    return nullptr;
}

// Comprehensive SSBU Primary Spirit Collection (Neutral, Attack, Defense, Grab across Legend/Ace/Advanced/Novice)
const std::vector<PrimarySpiritPreset>& GetPrimarySpiritCatalog() {
    static const std::vector<PrimarySpiritPreset> kPrimary = {
        {0,    "No Spirit (Vanilla 0 / 0)",      SpiritType::Neutral, 0,    0,    3, "None",     "Super Smash Bros."},
        {1297, "Galeem",                         SpiritType::Neutral, 2500, 2500, 3, "Legend",   "Super Smash Bros."},
        {1298, "Dharkon",                        SpiritType::Neutral, 2500, 2500, 3, "Legend",   "Super Smash Bros."},
        {1302, "Tabuu",                          SpiritType::Neutral, 2650, 2350, 3, "Legend",   "Super Smash Bros."},
        {48,   "Bowser (Wedding)",               SpiritType::Attack,  2800, 2200, 3, "Legend",   "Super Mario"},
        {21,   "Metal Mario",                    SpiritType::Defense, 1800, 3200, 1, "Ace",      "Super Mario"},
        {35,   "Paper Mario",                    SpiritType::Neutral, 2400, 2400, 3, "Ace",      "Super Mario"},
        {58,   "Geno",                           SpiritType::Attack,  2650, 2350, 3, "Legend",   "Super Mario"},
        {64,   "Peachette",                      SpiritType::Grab,    2350, 2650, 3, "Ace",      "Super Mario"},
        {89,   "Funky Kong",                     SpiritType::Attack,  2750, 2250, 3, "Ace",      "Donkey Kong"},
        {118,  "Zelda (Breath of the Wild)",     SpiritType::Defense, 2250, 2750, 3, "Legend",   "The Legend of Zelda"},
        {130,  "Daruk",                          SpiritType::Defense, 2100, 2900, 2, "Ace",      "The Legend of Zelda"},
        {131,  "Mipha",                          SpiritType::Grab,    2300, 2700, 3, "Ace",      "The Legend of Zelda"},
        {132,  "Revali",                         SpiritType::Attack,  2750, 2250, 2, "Ace",      "The Legend of Zelda"},
        {133,  "Urbosa",                         SpiritType::Attack,  2800, 2200, 2, "Ace",      "The Legend of Zelda"},
        {142,  "Calamity Ganon",                 SpiritType::Attack,  3100, 1900, 1, "Legend",   "The Legend of Zelda"},
        {155,  "Fierce Deity Link",              SpiritType::Attack,  3050, 1950, 1, "Legend",   "The Legend of Zelda"},
        {165,  "Midna (True Form)",              SpiritType::Grab,    2550, 2450, 3, "Legend",   "The Legend of Zelda"},
        {205,  "Queen Metroid",                  SpiritType::Grab,    2650, 2350, 3, "Legend",   "Metroid"},
        {214,  "Dark Suit Samus",                SpiritType::Defense, 2200, 2800, 2, "Ace",      "Metroid"},
        {248,  "Marx (True Form)",               SpiritType::Attack,  2900, 2100, 3, "Legend",   "Kirby"},
        {254,  "Galacta Knight",                 SpiritType::Defense, 2700, 2300, 2, "Legend",   "Kirby"},
        {280,  "Wolf O'Donnell",                 SpiritType::Attack,  2700, 2300, 2, "Ace",      "Star Fox"},
        {312,  "Gyarados",                       SpiritType::Attack,  2700, 2300, 2, "Ace",      "Pokemon"},
        {415,  "Rayquaza",                       SpiritType::Neutral, 2650, 2350, 3, "Legend",   "Pokemon"},
        {438,  "Arceus",                         SpiritType::Neutral, 2500, 2500, 3, "Legend",   "Pokemon"},
        {472,  "Zygarde (Complete Forme)",       SpiritType::Defense, 2150, 2850, 3, "Legend",   "Pokemon"},
        {498,  "Zacian & Zamazenta",             SpiritType::Neutral, 2500, 2500, 3, "Legend",   "Pokemon"},
        {514,  "Giygas",                         SpiritType::Defense, 2200, 2800, 3, "Legend",   "EarthBound"},
        {522,  "Claus",                          SpiritType::Neutral, 2550, 2450, 3, "Legend",   "EarthBound"},
        {552,  "Black Shadow",                   SpiritType::Attack,  2850, 2150, 2, "Ace",      "F-Zero"},
        {605,  "Lyn (Blade Lord)",               SpiritType::Attack,  2850, 2150, 2, "Legend",   "Fire Emblem"},
        {612,  "Black Knight",                   SpiritType::Defense, 2400, 2600, 2, "Legend",   "Fire Emblem"},
        {631,  "Sirius",                         SpiritType::Attack,  2750, 2250, 3, "Legend",   "Fire Emblem"},
        {648,  "Caeda (Princess of Talys)",      SpiritType::Grab,    2400, 2600, 3, "Legend",   "Fire Emblem"},
        {678,  "Hades (Final Form)",             SpiritType::Attack,  2950, 2050, 2, "Legend",   "Kid Icarus"},
        {712,  "Medusa (Queen of the Underworld)",SpiritType::Attack, 2800, 2200, 2, "Legend",   "Kid Icarus"},
        {750,  "Super Sonic",                    SpiritType::Grab,    2600, 2400, 3, "Legend",   "Sonic the Hedgehog"},
        {754,  "Shadow (Super)",                 SpiritType::Attack,  2750, 2250, 2, "Legend",   "Sonic the Hedgehog"},
        {782,  "King Pikmin",                    SpiritType::Grab,    2300, 2700, 3, "Ace",      "Pikmin"},
        {854,  "Chibi-Robo (Super)",             SpiritType::Grab,    2600, 2100, 2, "Ace",      "Chibi-Robo!"},
        {892,  "Shulk & Fiora (Mechon)",         SpiritType::Neutral, 2550, 2450, 3, "Legend",   "Xenoblade Chronicles"},
        {902,  "Rex (Master Driver)",            SpiritType::Defense, 2450, 2550, 3, "Legend",   "Xenoblade Chronicles"},
        {988,  "Akuma",                          SpiritType::Attack,  3400, 1600, 0, "Legend",   "Street Fighter"},
        {990,  "M. Bison",                       SpiritType::Defense, 2550, 2450, 3, "Legend",   "Street Fighter"},
        {1012, "Proto Man",                      SpiritType::Defense, 2350, 2650, 3, "Ace",      "Mega Man"},
        {1015, "Zero (Z-Saber)",                 SpiritType::Attack,  2900, 2100, 2, "Legend",   "Mega Man"},
        {1052, "Soma Cruz",                      SpiritType::Neutral, 2550, 2450, 3, "Legend",   "Castlevania"},
        {1055, "Alucard",                        SpiritType::Attack,  2650, 2350, 3, "Legend",   "Castlevania"},
        {1058, "Dracula (2nd Form)",             SpiritType::Grab,    2700, 2300, 3, "Legend",   "Castlevania"},
        {1112, "Big Boss",                       SpiritType::Defense, 2300, 2700, 3, "Legend",   "Metal Gear"},
        {1115, "The Boss",                       SpiritType::Grab,    2550, 2450, 3, "Legend",   "Metal Gear"},
        {1122, "Gray Fox",                       SpiritType::Attack,  2700, 2300, 3, "Ace",      "Metal Gear"},
        {1168, "Rodin, the Infinite One",        SpiritType::Attack,  3000, 2000, 2, "Legend",   "Bayonetta"},
        {1201, "Toad",                           SpiritType::Grab,    2100, 2600, 3, "Advanced", "Super Mario"},
        {1240, "Digby",                          SpiritType::Neutral, 2400, 2200, 3, "Advanced", "Animal Crossing"},
        {1255, "DJ K.K.",                        SpiritType::Neutral, 2150, 2850, 3, "Ace",      "Animal Crossing"},
        {1318, "Phantom Thieves of Hearts",      SpiritType::Grab,    2600, 2400, 3, "Legend",   "Persona"},
        {1332, "Hero's Comrades",                SpiritType::Defense, 2450, 2550, 3, "Legend",   "Dragon Quest"},
        {1355, "Gruntilda",                      SpiritType::Defense, 2400, 2600, 3, "Legend",   "Banjo-Kazooie"},
        {1378, "Geese Howard",                   SpiritType::Attack,  2850, 2150, 3, "Legend",   "Fatal Fury"},
        {1395, "Edelgard (Five Years Later)",    SpiritType::Defense, 2700, 2300, 2, "Legend",   "Fire Emblem"},
        {1396, "Dimitri (Five Years Later)",     SpiritType::Attack,  2850, 2150, 2, "Legend",   "Fire Emblem"},
        {1397, "Claude (Five Years Later)",      SpiritType::Grab,    2600, 2400, 3, "Legend",   "Fire Emblem"},
        {1442, "Ender Dragon",                   SpiritType::Attack,  2750, 2250, 2, "Legend",   "Minecraft"},
        {1465, "Sephiroth (One-Winged)",         SpiritType::Attack,  2950, 2050, 3, "Legend",   "Final Fantasy"},
        {1480, "Mythra (Awakened)",              SpiritType::Grab,    2700, 2300, 3, "Legend",   "Xenoblade Chronicles"},
        {1492, "Devil Jin",                      SpiritType::Attack,  2950, 2050, 2, "Legend",   "Tekken"},
        {1508, "Roxas",                          SpiritType::Attack,  2650, 2350, 3, "Legend",   "Kingdom Hearts"}
    };
    return kPrimary;
}

std::vector<const PrimarySpiritPreset*> FilterPrimarySpirits(const std::string& filter) {
    std::vector<const PrimarySpiritPreset*> out;
    std::string q = ToLower(filter);
    for (const auto& p : GetPrimarySpiritCatalog()) {
        if (!q.empty() && p.id != 0) {
            if (ToLower(p.name).find(q) == std::string::npos &&
                ToLower(p.series).find(q) == std::string::npos &&
                ToLower(SpiritTypeToString(p.type)).find(q) == std::string::npos) {
                continue;
            }
        }
        out.push_back(&p);
    }
    return out;
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

// Complete 89-Fighter SSBU Roster with NTAG215 Character IDs
const std::vector<FighterInfo>& GetFighterCatalog() {
    static const std::vector<FighterInfo> kFighters = {
        {0x0000, "01",  "Mario",                1.00f, 1.05f, 0.35f, 1.15f},
        {0x0008, "02",  "Donkey Kong",          1.32f, 1.00f, 0.05f, 1.40f},
        {0x0100, "03",  "Link",                 1.06f, 0.90f, 0.75f, 1.00f},
        {0x0500, "04",  "Samus",                1.10f, 0.95f, 0.85f, 1.20f},
        {0x0501, "04E", "Dark Samus",           1.10f, 0.95f, 0.85f, 1.20f},
        {0x0003, "05",  "Yoshi",                1.06f, 1.38f, 0.35f, 1.05f},
        {0x1F00, "06",  "Kirby",                0.78f, 1.10f, 0.15f, 1.05f},
        {0x0580, "07",  "Fox",                  0.77f, 1.35f, 0.30f, 1.00f},
        {0x1919, "08",  "Pikachu",              0.79f, 1.30f, 0.45f, 1.15f},
        {0x0001, "09",  "Luigi",                0.98f, 0.85f, 0.30f, 1.45f},
        {0x2280, "10",  "Ness",                 0.94f, 1.15f, 0.60f, 1.30f},
        {0x0600, "11",  "Captain Falcon",       1.06f, 1.25f, 0.00f, 1.20f},
        {0x1996, "12",  "Jigglypuff",           0.68f, 1.45f, 0.00f, 0.90f},
        {0x0002, "13",  "Peach",                0.90f, 1.25f, 0.40f, 1.10f},
        {0x0013, "13E", "Daisy",                0.90f, 1.25f, 0.40f, 1.10f},
        {0x0005, "14",  "Bowser",               1.40f, 0.95f, 0.25f, 1.35f},
        {0x2300, "15",  "Ice Climbers",         0.92f, 0.95f, 0.35f, 1.25f},
        {0x0102, "16",  "Sheik",                0.78f, 1.35f, 0.40f, 1.20f},
        {0x0101, "17",  "Zelda",                0.86f, 0.90f, 0.70f, 1.05f},
        {0x000E, "18",  "Dr. Mario",            1.00f, 0.90f, 0.40f, 1.15f},
        {0x19AC, "19",  "Pichu",                0.65f, 1.32f, 0.45f, 1.15f},
        {0x0581, "20",  "Falco",                0.82f, 1.25f, 0.40f, 1.15f},
        {0x2100, "21",  "Marth",                0.90f, 1.15f, 0.05f, 1.05f},
        {0x2101, "21E", "Lucina",               0.90f, 1.15f, 0.05f, 1.05f},
        {0x0104, "22",  "Young Link",           0.88f, 1.10f, 0.85f, 1.05f},
        {0x0103, "23",  "Ganondorf",            1.25f, 0.75f, 0.00f, 1.15f},
        {0x199A, "24",  "Mewtwo",               0.79f, 1.25f, 0.55f, 1.20f},
        {0x2102, "25",  "Roy",                  0.96f, 1.25f, 0.05f, 1.15f},
        {0x2105, "25E", "Chrom",                0.96f, 1.25f, 0.05f, 1.10f},
        {0x0740, "26",  "Mr. Game & Watch",     0.75f, 1.28f, 0.45f, 1.20f},
        {0x1F02, "27",  "Meta Knight",          0.80f, 1.25f, 0.05f, 1.00f},
        {0x0640, "28",  "Pit",                  0.96f, 1.15f, 0.55f, 1.15f},
        {0x0641, "28E", "Dark Pit",             0.96f, 1.15f, 0.55f, 1.15f},
        {0x0502, "29",  "Zero Suit Samus",      0.80f, 1.35f, 0.35f, 1.10f},
        {0x0007, "30",  "Wario",                1.08f, 1.30f, 0.20f, 1.15f},
        {0x3240, "31",  "Snake",                1.08f, 0.90f, 0.85f, 1.25f},
        {0x2103, "32",  "Ike",                  1.12f, 1.05f, 0.10f, 1.15f},
        {0x1907, "33",  "Pokemon Trainer (Squirtle)", 0.85f, 1.20f, 0.35f, 1.25f},
        {0x1902, "34",  "Ivysaur",              0.96f, 1.05f, 0.55f, 1.20f},
        {0x1906, "35",  "Charizard",            1.20f, 1.00f, 0.30f, 1.30f},
        {0x0009, "36",  "Diddy Kong",           0.92f, 1.15f, 0.55f, 1.25f},
        {0x2281, "37",  "Lucas",                0.94f, 1.15f, 0.65f, 1.25f},
        {0x3200, "38",  "Sonic",                0.86f, 1.35f, 0.20f, 1.05f},
        {0x1F01, "39",  "King Dedede",          1.35f, 0.80f, 0.45f, 1.15f},
        {0x0680, "40",  "Olimar",               0.79f, 1.05f, 0.75f, 1.30f},
        {0x1A04, "41",  "Lucario",              0.94f, 1.15f, 0.50f, 1.10f},
        {0x0781, "42",  "R.O.B.",               1.08f, 1.15f, 0.75f, 1.25f},
        {0x0105, "43",  "Toon Link",            0.91f, 1.15f, 0.80f, 1.10f},
        {0x0582, "44",  "Wolf",                 0.94f, 1.20f, 0.50f, 1.15f},
        {0x0180, "45",  "Villager",             0.92f, 1.10f, 0.85f, 1.10f},
        {0x3480, "46",  "Mega Man",             1.04f, 1.00f, 0.90f, 1.05f},
        {0x07C0, "47",  "Wii Fit Trainer",      0.96f, 1.10f, 0.65f, 1.10f},
        {0x0004, "48",  "Rosalina & Luma",      0.82f, 1.15f, 0.50f, 1.10f},
        {0x06C0, "49",  "Little Mac",           0.88f, 0.70f, 0.00f, 1.00f},
        {0x1B92, "50",  "Greninja",             0.88f, 1.30f, 0.40f, 1.10f},
        {0x0700, "51",  "Mii Brawler",          0.96f, 1.20f, 0.25f, 1.20f},
        {0x0701, "52",  "Mii Swordfighter",     1.00f, 1.10f, 0.55f, 1.10f},
        {0x0702, "53",  "Mii Gunner",           1.04f, 0.95f, 0.90f, 1.10f},
        {0x0642, "54",  "Palutena",             0.91f, 1.25f, 0.55f, 1.25f},
        {0x3340, "55",  "Pac-Man",              0.95f, 1.15f, 0.80f, 1.15f},
        {0x2104, "56",  "Robin",                0.95f, 1.00f, 0.75f, 1.10f},
        {0x2240, "57",  "Shulk",                0.98f, 1.20f, 0.05f, 1.10f},
        {0x0006, "58",  "Bowser Jr.",           1.10f, 1.10f, 0.65f, 1.15f},
        {0x0782, "59",  "Duck Hunt",            0.86f, 1.10f, 0.85f, 1.05f},
        {0x34C0, "60",  "Ryu",                  1.05f, 0.95f, 0.45f, 1.15f},
        {0x34C1, "60E", "Ken",                  1.05f, 1.00f, 0.35f, 1.15f},
        {0x3600, "61",  "Cloud",                1.02f, 1.20f, 0.35f, 1.00f},
        {0x2107, "62",  "Corrin",               0.98f, 1.10f, 0.35f, 1.10f},
        {0x3680, "63",  "Bayonetta",            0.81f, 1.25f, 0.30f, 1.00f},
        {0x0800, "64",  "Inkling",              0.94f, 1.25f, 0.50f, 1.20f},
        {0x0503, "65",  "Ridley",               1.09f, 1.10f, 0.35f, 1.20f},
        {0x3700, "66",  "Simon",                1.08f, 0.90f, 0.85f, 1.10f},
        {0x3701, "66E", "Richter",              1.08f, 0.90f, 0.85f, 1.10f},
        {0x0780, "67",  "King K. Rool",         1.38f, 0.85f, 0.65f, 1.25f},
        {0x0181, "68",  "Isabelle",             0.88f, 1.10f, 0.70f, 1.15f},
        {0x1C13, "69",  "Incineroar",           1.22f, 0.80f, 0.10f, 1.45f},
        {0x0015, "70",  "Piranha Plant",        1.15f, 0.95f, 0.60f, 1.15f},
        {0x3740, "71",  "Joker",                0.93f, 1.25f, 0.45f, 1.15f},
        {0x3780, "72",  "Hero",                 1.01f, 1.00f, 0.75f, 1.10f},
        {0x37C0, "73",  "Banjo & Kazooie",      1.06f, 1.05f, 0.65f, 1.15f},
        {0x3800, "74",  "Terry",                1.08f, 1.05f, 0.35f, 1.15f},
        {0x2106, "75",  "Byleth",               0.98f, 0.95f, 0.40f, 1.15f},
        {0x3840, "76",  "Min Min",              1.04f, 0.95f, 0.75f, 1.20f},
        {0x38C0, "77",  "Steve",                0.92f, 1.10f, 0.65f, 1.20f},
        {0x3601, "78",  "Sephiroth",            0.79f, 1.15f, 0.55f, 1.10f},
        {0x2241, "79",  "Pyra",                 0.98f, 1.15f, 0.35f, 1.15f},
        {0x2242, "80",  "Mythra",               0.92f, 1.38f, 0.25f, 1.15f},
        {0x3900, "81",  "Kazuya",               1.15f, 0.90f, 0.30f, 1.40f},
        {0x3940, "82",  "Sora",                 0.85f, 1.35f, 0.45f, 1.05f}
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
