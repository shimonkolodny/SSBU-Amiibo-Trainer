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

// Complete 91-Fighter SSBU Roster with Verbatim 64-Bit Amiibo IDs from
// jozz024/smash-amiibo-editor/resources/characters.json & N3evin/AmiiboAPI
const std::vector<FighterInfo>& GetFighterCatalog() {
    static const std::vector<FighterInfo> kFighters = {
        {0x0000, 0x0000000000000002ULL, "01",  "Mario",                      1.00f, 1.05f, 0.35f, 1.15f},
        {0x0008, 0x0008000000030002ULL, "02",  "Donkey Kong",                1.32f, 1.00f, 0.05f, 1.40f},
        {0x0100, 0x0100000000040002ULL, "03",  "Link",                       1.06f, 0.90f, 0.75f, 1.00f},
        {0x05C0, 0x05C0000000060002ULL, "04",  "Samus",                      1.10f, 0.95f, 0.85f, 1.20f},
        {0x05C3, 0x05C3000003800002ULL, "04E", "Dark Samus",                 1.10f, 0.95f, 0.85f, 1.20f},
        {0x0003, 0x0003000000020002ULL, "05",  "Yoshi",                      1.06f, 1.38f, 0.35f, 1.05f},
        {0x1F00, 0x1F000000000A0002ULL, "06",  "Kirby",                      0.78f, 1.10f, 0.15f, 1.05f},
        {0x0580, 0x0580000000050002ULL, "07",  "Fox",                        0.77f, 1.35f, 0.30f, 1.00f},
        {0x1919, 0x1919000000090002ULL, "08",  "Pikachu",                    0.79f, 1.30f, 0.45f, 1.15f},
        {0x0001, 0x00010000000C0002ULL, "09",  "Luigi",                      0.98f, 0.85f, 0.30f, 1.45f},
        {0x2280, 0x22800000002C0002ULL, "10",  "Ness",                       0.94f, 1.15f, 0.60f, 1.30f},
        {0x0600, 0x0600000000120002ULL, "11",  "Captain Falcon",             1.06f, 1.25f, 0.00f, 1.20f},
        {0x1927, 0x1927000000260002ULL, "12",  "Jigglypuff",                 0.68f, 1.45f, 0.00f, 0.90f},
        {0x0002, 0x0002000000010002ULL, "13",  "Peach",                      0.90f, 1.25f, 0.40f, 1.10f},
        {0x0013, 0x0013000002660102ULL, "13E", "Daisy",                      0.90f, 1.25f, 0.40f, 1.10f},
        {0x0005, 0x0005000000140002ULL, "14",  "Bowser",                     1.40f, 0.95f, 0.25f, 1.35f},
        {0x078F, 0x078F000003810002ULL, "15",  "Ice Climbers",               0.92f, 0.95f, 0.35f, 1.25f},
        {0x0112, 0x0101010000170002ULL, "16",  "Sheik",                      0.78f, 1.35f, 0.40f, 1.20f},
        {0x0101, 0x01010000000E0002ULL, "17",  "Zelda",                      0.86f, 0.90f, 0.70f, 1.05f},
        {0x000E, 0x0000010000190002ULL, "18",  "Dr. Mario",                  1.00f, 0.90f, 0.40f, 1.15f},
        {0x19AC, 0x19AC000003850002ULL, "19",  "Pichu",                      0.65f, 1.32f, 0.45f, 1.15f},
        {0x0581, 0x05810000001C0002ULL, "20",  "Falco",                      0.82f, 1.25f, 0.40f, 1.15f},
        {0x2100, 0x21000000000B0002ULL, "21",  "Marth",                      0.90f, 1.15f, 0.05f, 1.05f},
        {0x2102, 0x2102000000290002ULL, "21E", "Lucina",                     0.90f, 1.15f, 0.05f, 1.05f},
        {0x0104, 0x01000000037C0002ULL, "22",  "Young Link",                 0.88f, 1.10f, 0.85f, 1.05f},
        {0x0102, 0x01020100001B0002ULL, "23",  "Ganondorf",                  1.25f, 0.75f, 0.00f, 1.15f},
        {0x1996, 0x19960000023D0002ULL, "24",  "Mewtwo",                     0.79f, 1.25f, 0.55f, 1.20f},
        {0x2104, 0x2104000002520002ULL, "25",  "Roy",                        0.96f, 1.25f, 0.05f, 1.15f},
        {0x2108, 0x21080000036F1202ULL, "25E", "Chrom",                      0.96f, 1.25f, 0.05f, 1.10f},
        {0x0780, 0x07800000002D0002ULL, "26",  "Mr. Game & Watch",           0.75f, 1.28f, 0.45f, 1.20f},
        {0x1F01, 0x1F01000000270002ULL, "27",  "Meta Knight",                0.80f, 1.25f, 0.05f, 1.00f},
        {0x0740, 0x0740000000100002ULL, "28",  "Pit",                        0.96f, 1.15f, 0.55f, 1.15f},
        {0x0741, 0x0741000000200002ULL, "28E", "Dark Pit",                   0.96f, 1.15f, 0.55f, 1.15f},
        {0x05C1, 0x05C00100001D0002ULL, "29",  "Zero Suit Samus",            0.80f, 1.35f, 0.35f, 1.10f},
        {0x0007, 0x00070000001A0002ULL, "30",  "Wario",                      1.08f, 1.30f, 0.20f, 1.15f},
        {0x3780, 0x37800000038A0002ULL, "31",  "Snake",                      1.08f, 0.90f, 0.85f, 1.25f},
        {0x2101, 0x2101000000180002ULL, "32",  "Ike",                        1.12f, 1.05f, 0.10f, 1.15f},
        {0x1D40, 0x1D40000003870002ULL, "33",  "Pokemon Trainer",            0.96f, 1.15f, 0.40f, 1.25f},
        {0x1907, 0x1907000003840002ULL, "33S", "Pokemon Trainer (Squirtle)", 0.85f, 1.20f, 0.35f, 1.25f},
        {0x1902, 0x1902000003830002ULL, "34",  "Ivysaur",                    0.96f, 1.05f, 0.55f, 1.20f},
        {0x1906, 0x1906000000240002ULL, "35",  "Charizard",                  1.20f, 1.00f, 0.30f, 1.30f},
        {0x0009, 0x00090000000D0002ULL, "36",  "Diddy Kong",                 0.92f, 1.15f, 0.55f, 1.25f},
        {0x2281, 0x2281000002510002ULL, "37",  "Lucas",                      0.94f, 1.15f, 0.65f, 1.25f},
        {0x3200, 0x3200000000300002ULL, "38",  "Sonic",                      0.86f, 1.35f, 0.20f, 1.05f},
        {0x1F02, 0x1F02000000280002ULL, "39",  "King Dedede",                1.35f, 0.80f, 0.45f, 1.15f},
        {0x0640, 0x06400100001E0002ULL, "40",  "Olimar",                     0.79f, 1.05f, 0.75f, 1.30f},
        {0x1AC0, 0x1AC0000000110002ULL, "41",  "Lucario",                    0.94f, 1.15f, 0.50f, 1.10f},
        {0x0781, 0x0781000000330002ULL, "42",  "R.O.B.",                     1.08f, 1.15f, 0.75f, 1.25f},
        {0x0105, 0x0100010000160002ULL, "43",  "Toon Link",                  0.91f, 1.15f, 0.80f, 1.10f},
        {0x0584, 0x05840000037E0002ULL, "44",  "Wolf",                       0.94f, 1.20f, 0.50f, 1.15f},
        {0x0180, 0x0180000000080002ULL, "45",  "Villager",                   0.92f, 1.10f, 0.85f, 1.10f},
        {0x3480, 0x3480000000310002ULL, "46",  "Mega Man",                   1.04f, 1.00f, 0.90f, 1.05f},
        {0x0700, 0x0700000000070002ULL, "47",  "Wii Fit Trainer",            0.96f, 1.10f, 0.65f, 1.10f},
        {0x0004, 0x0004010000130002ULL, "48",  "Rosalina & Luma",            0.82f, 1.15f, 0.50f, 1.10f},
        {0x06C0, 0x06C00000000F0002ULL, "49",  "Little Mac",                 0.88f, 0.70f, 0.00f, 1.00f},
        {0x1B92, 0x1B92000000250002ULL, "50",  "Greninja",                   0.88f, 1.30f, 0.40f, 1.10f},
        {0x07C0, 0x07C0000000210002ULL, "51",  "Mii Brawler",                0.96f, 1.20f, 0.25f, 1.20f},
        {0x07C1, 0x07C0010000220002ULL, "52",  "Mii Swordfighter",           1.00f, 1.10f, 0.55f, 1.10f},
        {0x07C2, 0x07C0020000230002ULL, "53",  "Mii Gunner",                 1.04f, 0.95f, 0.90f, 1.10f},
        {0x0742, 0x07420000001F0002ULL, "54",  "Palutena",                   0.91f, 1.25f, 0.55f, 1.25f},
        {0x3340, 0x3340000000320002ULL, "55",  "Pac-Man",                    0.95f, 1.15f, 0.80f, 1.15f},
        {0x2103, 0x21030000002A0002ULL, "56",  "Robin",                      0.95f, 1.00f, 0.75f, 1.10f},
        {0x2240, 0x22400000002B0002ULL, "57",  "Shulk",                      0.98f, 1.20f, 0.05f, 1.10f},
        {0x0006, 0x0006000000150002ULL, "58",  "Bowser Jr.",                 1.10f, 1.10f, 0.65f, 1.15f},
        {0x0782, 0x07820000002F0002ULL, "59",  "Duck Hunt",                  0.86f, 1.10f, 0.85f, 1.05f},
        {0x34C0, 0x34C0000002530002ULL, "60",  "Ryu",                        1.05f, 0.95f, 0.45f, 1.15f},
        {0x34C1, 0x34C1000003890002ULL, "60E", "Ken",                        1.05f, 1.00f, 0.35f, 1.15f},
        {0x3600, 0x3600000002590002ULL, "61",  "Cloud",                      1.02f, 1.20f, 0.35f, 1.00f},
        {0x2105, 0x21050000025A0002ULL, "62",  "Corrin",                     0.98f, 1.10f, 0.35f, 1.10f},
        {0x3240, 0x32400000025B0002ULL, "63",  "Bayonetta",                  0.81f, 1.25f, 0.30f, 1.00f},
        {0x0800, 0x0800010003820002ULL, "64",  "Inkling",                    0.94f, 1.25f, 0.50f, 1.20f},
        {0x05C2, 0x05C20000037F0002ULL, "65",  "Ridley",                     1.09f, 1.10f, 0.35f, 1.20f},
        {0x37C0, 0x37C00000038B0002ULL, "66",  "Simon",                      1.08f, 0.90f, 0.85f, 1.10f},
        {0x37C1, 0x37C10000038C0002ULL, "66E", "Richter",                    1.08f, 0.90f, 0.85f, 1.10f},
        {0x00C0, 0x00C00000037B0002ULL, "67",  "King K. Rool",               1.38f, 0.85f, 0.65f, 1.25f},
        {0x0181, 0x01810000037D0002ULL, "68",  "Isabelle",                   0.88f, 1.10f, 0.70f, 1.15f},
        {0x1BD7, 0x1BD7000003860002ULL, "69",  "Incineroar",                 1.22f, 0.80f, 0.10f, 1.45f},
        {0x0024, 0x00240000038D0002ULL, "70",  "Piranha Plant",              1.15f, 0.95f, 0.60f, 1.15f},
        {0x3A00, 0x3A00000003A10002ULL, "71",  "Joker",                      0.93f, 1.25f, 0.45f, 1.15f},
        {0x3640, 0x3640000003A20002ULL, "72",  "Hero",                       1.01f, 1.00f, 0.75f, 1.10f},
        {0x3B40, 0x3B40000003A30002ULL, "73",  "Banjo & Kazooie",            1.06f, 1.05f, 0.65f, 1.15f},
        {0x3C80, 0x3C80000003A40002ULL, "74",  "Terry",                      1.08f, 1.05f, 0.35f, 1.15f},
        {0x210B, 0x210B000003A50002ULL, "75",  "Byleth",                     0.98f, 0.95f, 0.40f, 1.15f},
        {0x0A40, 0x0A400000041D0002ULL, "76",  "Min Min",                    1.04f, 0.95f, 0.75f, 1.20f},
        {0x3DC0, 0x3DC0000004220002ULL, "77",  "Steve",                      0.92f, 1.10f, 0.65f, 1.20f},
        {0x3DC1, 0x3DC1000004230002ULL, "77E", "Alex",                       0.92f, 1.10f, 0.65f, 1.20f},
        {0x3601, 0x3601000004210002ULL, "78",  "Sephiroth",                  0.79f, 1.15f, 0.55f, 1.10f},
        {0x2241, 0x22410000041E0002ULL, "79",  "Pyra",                       0.98f, 1.15f, 0.35f, 1.15f},
        {0x2242, 0x22420000041F0002ULL, "80",  "Mythra",                     0.92f, 1.38f, 0.25f, 1.15f},
        {0x33C0, 0x33C0000004200002ULL, "81",  "Kazuya",                     1.15f, 0.90f, 0.30f, 1.40f},
        {0x3F00, 0x3F000000042E0002ULL, "82",  "Sora",                       0.85f, 1.35f, 0.45f, 1.05f}
    };
    return kFighters;
}

// Complete 162-entry official SSBU Amiibo 64-bit ID table from jozz024/smash-amiibo-editor/resources/characters.json
struct VariantAmiiboMap {
    uint64_t amiibo_id;
    uint16_t fighter_char_id;
};

static const VariantAmiiboMap kCanonicalSsbuAmiiboVariants[] = {
    {2ULL,                    0x0000}, // Mario
    {3408130ULL,              0x0000}, // Mario: Super Mario
    {3932418ULL,              0x0000}, // Mario - Gold Edition
    {3997954ULL,              0x0000}, // Mario - Silver Edition
    {37225986ULL,             0x0000}, // 8-Bit Mario Classic Color
    {37291522ULL,             0x0000}, // 8-Bit Mario Modern Color
    {57737474ULL,             0x0000}, // Mario - Wedding
    {12945456898ULL,          0x0000}, // Mario - Power Up Band
    {1099513266178ULL,        0x000E}, // Dr. Mario
    {3298596094210ULL,        0x0000}, // Mario - Cat
    {281474977497090ULL,      0x0001}, // Luigi
    {281474980184322ULL,      0x0001}, // Luigi: Super Mario
    {281487922233090ULL,      0x0001}, // Luigi - Power Up Band
    {562949953486850ULL,      0x0002}, // Peach
    {562949956960514ULL,      0x0002}, // Peach: Super Mario
    {562950011224322ULL,      0x0002}, // Peach - Wedding
    {562962899009282ULL,      0x0002}, // Peach - Power Up Band
    {564049526325506ULL,      0x0002}, // Peach - Cat
    {844424930263042ULL,      0x0003}, // Yoshi
    {844424933736706ULL,      0x0003}, // Yoshi: Super Mario
    {844437875851010ULL,      0x0003}, // Yoshi - Power Up Band
    {845533035954946ULL,      0x0003}, // Green Yarn Yoshi
    {845533036020482ULL,      0x0003}, // Pink Yarn Yoshi
    {845533036086018ULL,      0x0003}, // Light Blue Yarn Yoshi
    {845533069312770ULL,      0x0003}, // Mega Yarn Yoshi
    {1125899946819842ULL,     0x0004}, // Rosalina
    {1126999419715586ULL,     0x0004}, // Rosalina & Luma
    {1407374884864002ULL,     0x0005}, // Bowser
    {1407374887289090ULL,     0x0005}, // Bowser: Super Mario
    {1407374941421826ULL,     0x0005}, // Bowser - Wedding
    {1687750385993474ULL,     0x0005}, // Hammer Slam Bowser
    {1688849861640194ULL,     0x0006}, // Bowser Jr.
    {1970324838678530ULL,     0x0007}, // Wario
    {1970324877017346ULL,     0x0007}, // Wario: Super Mario
    {2251799813881858ULL,     0x0008}, // Donkey Kong
    {2251799853793538ULL,     0x0008}, // Donkey Kong: Super Mario
    {2532175316190978ULL,     0x0008}, // Turbo Charge Donkey Kong
    {2533274791247874ULL,     0x0009}, // Diddy Kong
    {2533274830569730ULL,     0x0009}, // Diddy Kong: Super Mario
    {5348024597741826ULL,     0x0013}, // Daisy
    {5348024615829506ULL,     0x0013}, // Daisy: Super Mario
    {5348037503155970ULL,     0x0013}, // Daisy - Power Up Band
    {10133099221155842ULL,    0x0024}, // Piranha Plant
    {54043195586838530ULL,    0x00C0}, // King K. Rool
    {72057594038190082ULL,    0x0100}, // Link
    {72057594093177090ULL,    0x0100}, // Link - Ocarina of Time
    {72057594093242626ULL,    0x0100}, // Link - Majora's Mask
    {72057594093308162ULL,    0x0100}, // Link - Twilight Princess
    {72057594093373698ULL,    0x0100}, // Link - Skyward Sword
    {72057594093439234ULL,    0x0100}, // 8-Bit Link
    {72057594093701378ULL,    0x0100}, // Link - Archer
    {72057594093766914ULL,    0x0100}, // Link - Rider
    {72057594096386050ULL,    0x0104}, // Young Link
    {72057594098288898ULL,    0x0104}, // Link - Link's Awakening
    {72058693550997506ULL,    0x0105}, // Toon Link
    {72058693605132546ULL,    0x0105}, // Toon Link - The Wind Waker
    {72339069015556098ULL,    0x0101}, // Zelda
    {72339069070346498ULL,    0x0101}, // Toon Zelda - The Wind Waker
    {72339069070608642ULL,    0x0101}, // Zelda: The Legend of Zelda
    {72340168527773698ULL,    0x0112}, // Sheik
    {72342367617943810ULL,    0x0101}, // Zelda & Loftwing
    {72621643504746498ULL,    0x0102}, // Ganondorf
    {108086391057416194ULL,   0x0180}, // Villager
    {108367866072073474ULL,   0x0181}, // Isabelle - Summer Outfit
    {108367866092126210ULL,   0x0181}, // Isabelle
    {108372268436555010ULL,   0x0181}, // Isabelle: Animal Crossing
    {108367870359241986ULL,   0x0181}, // Isabelle - Character Parfait
    {108368965582914818ULL,   0x0181}, // Isabelle - Winter Outfit
    {108368969851995394ULL,   0x0181}, // Isabelle - Winter
    {108370069370307842ULL,   0x0181}, // Isabelle - Kimono
    {108371168887571714ULL,   0x0181}, // Isabelle - Dress
    {108373367949559042ULL,   0x0181}, // Isabelle - Sweater
    {396316767208931330ULL,   0x0580}, // Fox
    {396598242187149314ULL,   0x0581}, // Falco
    {397442667174035458ULL,   0x0584}, // Wolf
    {414331165718478850ULL,   0x05C0}, // Samus
    {414331165775041282ULL,   0x05C0}, // Samus Aran
    {414331165786379010ULL,   0x05C0}, // Samus - Metroid Dread
    {414332265231613954ULL,   0x05C1}, // Zero Suit Samus
    {414894115730161666ULL,   0x05C2}, // Ridley
    {415175590706937858ULL,   0x05C3}, // Dark Samus
    {432345564228747266ULL,   0x0600}, // Captain Falcon
    {450361062250643458ULL,   0x0640}, // Olimar
    {486388759756996610ULL,   0x06C0}, // Little Mac
    {504403158265954306ULL,   0x0700}, // Wii Fit Trainer
    {522417556776026114ULL,   0x0740}, // Pit
    {522699031753785346ULL,   0x0741}, // Dark Pit
    {522980506730430466ULL,   0x0742}, // Palutena
    {540431955287408642ULL,   0x0780}, // Mr. Game & Watch
    {540713430264184834ULL,   0x0781}, // R.O.B. - Famicom
    {540713430264512514ULL,   0x0781}, // R.O.B. - NES
    {540994905240961026ULL,   0x0782}, // Duck Hunt
    {544654079993905154ULL,   0x078F}, // Ice Climbers
    {558446353796104194ULL,   0x07C0}, // Mii Brawler
    {558447453307797506ULL,   0x07C1}, // Mii Swordfighter
    {558448552819490818ULL,   0x07C2}, // Mii Gunner
    {576461851819115522ULL,   0x0800}, // Inkling Girl
    {576461851854832642ULL,   0x0800}, // Inkling Girl - Lime Green
    {576461851872265218ULL,   0x0800}, // Inkling Girl - Neon Pink
    {576461851873902594ULL,   0x0800}, // Inkling
    {576462951330808834ULL,   0x0800}, // Inkling Boy
    {576462951366525954ULL,   0x0800}, // Inkling Boy - Purple
    {576462951383958530ULL,   0x0800}, // Inkling Boy - Neon Green
    {576464050842502146ULL,   0x0800}, // Inkling Squid
    {576464050878219266ULL,   0x0800}, // Inkling Squid - Orange
    {576464050895651842ULL,   0x0800}, // Inkling Squid - Neon Purple
    {1802002800960536578ULL,  0x1902}, // Ivysaur
    {1803128700810821634ULL,  0x1906}, // Charizard
    {1803410175844155394ULL,  0x1907}, // Squirtle
    {1808476725366554626ULL,  0x1919}, // Pikachu
    {1812417375042404354ULL,  0x1927}, // Jigglypuff
    {1843661097492348930ULL,  0x1996}, // Mewtwo
    {1849853547001479170ULL,  0x19AC}, // Pichu
    {1927540640515686402ULL,  0x1AC0}, // Lucario
    {1986650385626234882ULL,  0x1B92}, // Greninja
    {2006072159075958786ULL,  0x1BD7}, // Incineroar
    {2107684625668571138ULL,  0x1D40}, // Pokemon Trainer
    {2233785415176421378ULL,  0x1F00}, // Kirby
    {2233785415214828546ULL,  0x1F00}, // Kirby: Kirby
    {2234066890155032578ULL,  0x1F01}, // Meta Knight
    {2234066890191604738ULL,  0x1F01}, // Meta Knight: Kirby
    {2234348365131808770ULL,  0x1F02}, // King Dedede
    {2234348365168380930ULL,  0x1F02}, // King Dedede: Kirby
    {2377900603252342786ULL,  0x2100}, // Marth
    {2378182078229905410ULL,  0x2101}, // Ike
    {2378463553207730178ULL,  0x2102}, // Lucina
    {2378745028184506370ULL,  0x2103}, // Robin
    {2379026503197392898ULL,  0x2104}, // Roy
    {2379307978174627842ULL,  0x2105}, // Corrin
    {2379309077703622658ULL,  0x2105}, // Corrin - Player 2
    {2380152403122917890ULL,  0x2108}, // Chrom
    {2380152403124551682ULL,  0x2108}, // Chrom: Fire Emblem
    {2380996828056584194ULL,  0x210B}, // Byleth
    {2467972595801849858ULL,  0x2240}, // Shulk
    {2485986994311397378ULL,  0x2280}, // Ness
    {2486268469324087298ULL,  0x2281}, // Lucas
    {3602879701899542530ULL,  0x3200}, // Sonic
    {3620894100445396994ULL,  0x3240}, // Bayonetta
    {3620895199974391810ULL,  0x3240}, // Bayonetta - Player 2
    {3692951694447083522ULL,  0x3340}, // Pac-Man
    {3783023686994427906ULL,  0x3480}, // Mega Man
    {3783023687030538242ULL,  0x3480}, // Mega Man - Gold Edition
    {3783023687049483522ULL,  0x3480}, // Mega Man: Megaman
    {3801038085539692546ULL,  0x34C0}, // Ryu
    {3801319560536719362ULL,  0x34C1}, // Ken
    {3891110078087495682ULL,  0x3600}, // Cloud
    {3891111177616490498ULL,  0x3600}, // Cloud - Player 2
    {3909124476618539010ULL,  0x3640}, // Hero
    {3999196469164376066ULL,  0x3780}, // Snake
    {4017210867673923586ULL,  0x37C0}, // Simon
    {4017492342650699778ULL,  0x37C1}, // Richter
    {4179340454260703234ULL,  0x3A00}, // Joker
    {4269412446808244226ULL,  0x3B40}, // Banjo & Kazooie
    {4359484439355719682ULL,  0x3C80}, // Terry
    {738590338957770754ULL,   0x0A40}, // Min Min
    {4449556431911387138ULL,  0x3DC0}, // Steve
    {4449837906888163330ULL,  0x3DC1}, // Alex
    {3728980491531976706ULL,  0x33C0}, // Kazuya
    {3891391553094090754ULL,  0x3601}, // Sephiroth
    {2468254070844817410ULL,  0x2241}, // Pyra
    {2468535545821593602ULL,  0x2242}, // Mythra
    {4539628424459583490ULL,  0x3F00}  // Sora
};

bool IsCanonicalSsbuAmiiboId(uint64_t amiibo_id) {
    for (const auto& entry : kCanonicalSsbuAmiiboVariants) {
        if (entry.amiibo_id == amiibo_id) return true;
    }
    return false;
}

const FighterInfo* FindFighterByAmiiboId(uint64_t amiibo_id) {
    const auto& cat = GetFighterCatalog();
    // 1. Exact 64-bit match against primary catalog
    for (const auto& f : cat) {
        if (f.amiibo_id == amiibo_id) return &f;
    }
    // 2. Exact 64-bit match against all 162 official SSBU variant IDs in characters.json
    for (const auto& v : kCanonicalSsbuAmiiboVariants) {
        if (v.amiibo_id == amiibo_id) {
            return FindFighterById(v.fighter_char_id);
        }
    }
    // 3. Match by 16-bit Amiibo Model Number (bits 16..31)
    uint16_t model_no = static_cast<uint16_t>((amiibo_id >> 16) & 0xFFFFu);
    if (model_no != 0) {
        for (const auto& f : cat) {
            if (static_cast<uint16_t>((f.amiibo_id >> 16) & 0xFFFFu) == model_no) {
                return &f;
            }
        }
    }
    // 4. Fallback to top 16 bits (Game/Character ID at 0x1DC..0x1DD)
    uint16_t top16 = static_cast<uint16_t>((amiibo_id >> 48) & 0xFFFFu);
    return FindFighterById(top16);
}

const FighterInfo* FindFighterById(uint16_t char_id) {
    const auto& cat = GetFighterCatalog();
    for (const auto& f : cat) {
        if (f.char_id == char_id) return &f;
    }
    // Support legacy v2.1-v2.4 non-colliding fighter IDs so older files/CLI flags map cleanly
    switch (char_id) {
        case 0x0500: return FindFighterById(0x05C0); // Samus
        case 0x0501: return FindFighterById(0x05C3); // Dark Samus
        case 0x0502: return FindFighterById(0x05C1); // Zero Suit Samus
        case 0x0503: return FindFighterById(0x05C2); // Ridley
        case 0x2300: return FindFighterById(0x078F); // Ice Climbers
        case 0x0103: return FindFighterById(0x0102); // Ganondorf
        case 0x199A: return FindFighterById(0x1996); // Mewtwo
        case 0x0641: return FindFighterById(0x0741); // Dark Pit
        case 0x0642: return FindFighterById(0x0742); // Palutena
        case 0x0680: return FindFighterById(0x0640); // Olimar
        case 0x1A04: return FindFighterById(0x1AC0); // Lucario
        case 0x0582: return FindFighterById(0x0584); // Wolf
        case 0x0701: return FindFighterById(0x07C1); // Mii Swordfighter
        case 0x0702: return FindFighterById(0x07C2); // Mii Gunner
        case 0x2107: return FindFighterById(0x2105); // Corrin
        case 0x3680: return FindFighterById(0x3240); // Bayonetta
        case 0x3700: return FindFighterById(0x37C0); // Simon
        case 0x3701: return FindFighterById(0x37C1); // Richter
        case 0x1C13: return FindFighterById(0x1BD7); // Incineroar
        case 0x0015: return FindFighterById(0x0024); // Piranha Plant
        case 0x3740: return FindFighterById(0x3A00); // Joker
        case 0x3800: return FindFighterById(0x3C80); // Terry
        case 0x2106: return FindFighterById(0x210B); // Byleth
        case 0x3840: return FindFighterById(0x0A40); // Min Min
        case 0x38C0: return FindFighterById(0x3DC0); // Steve
        case 0x3900: return FindFighterById(0x33C0); // Kazuya
        case 0x3940: return FindFighterById(0x3F00); // Sora
        default: break;
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
