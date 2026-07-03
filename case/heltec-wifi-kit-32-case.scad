// ============================================================
//  Tragbares Gehäuse für Heltec WiFi Kit 32 V2 + LiPo-Akku
//  Parametrisch – alle Maße unten anpassen, dann F6 rendern.
//  Druck: PETG empfohlen, 2 Perimeter, 20-30% Infill
//
//  BOARD-MONTAGE AM DECKEL (4 Teile: bottom / lid / sep / slider)
//  Das Board hängt komplett am Deckel, gehalten an den LANGSEITEN:
//   - Eine FESTE C-Schiene am Deckel (eine Langseite).
//   - Die zweite Schiene ist ein SEPARATES SCHIEBETEIL, das auf
//     einer Schwalbenschwanz-Führung (Trapezprofil) am Deckel
//     sitzt und von der ANTENNEN-Seite (nicht-USB) aufgeschoben
//     wird.
//  Montage:
//   1. Board-Kante schräg in die feste Schiene einhängen,
//   2. Board flach an den Deckel anlegen (Display steigt dabei
//      senkrecht ins Fenster – kein Kollisionsproblem),
//   3. Schiebeschiene von der Antennen-Seite auf den Schwalben-
//      schwanz schieben, bis sie am blinden Nut-Ende anschlägt
//      und über die Rastnase klickt.
//  Im verschraubten Gehäuse begrenzt zusätzlich die Innenwand
//  den Rückweg des Schiebers.
// ============================================================

// ---------- PARAMETER: BITTE NACHMESSEN! ----------

// Board (Heltec WiFi Kit 32 V2) – Richtwerte, nachmessen!
board_l = 51.0;     // Länge (USB-Seite bis Antennen-Seite)
board_w = 25.5;     // Breite
board_t = 1.6;      // Platinendicke
pins_below = 3.0;   // Überstand Pin-Header / Lötstellen unter der Platine

// OLED-Display (Position von der USB-Kante / linken Kante aus messen!)
oled_x = 19.0;      // Abstand USB-Kante -> Display-Anfang
oled_y = 6.0;       // Abstand Board-Längskante -> Display-Anfang
oled_w = 25.0;      // Fensterbreite (in Board-Längsrichtung)
oled_h = 13.0;      // Fensterhöhe (quer)

// Micro-USB (Position der Buchsenmitte auf der kurzen Seite)
usb_w = 10.0;       // Aussparungsbreite
usb_h = 5.0;        // Aussparungshöhe
usb_z_lift = 1.0;   // Unterkante der Öffnung über der Platinen-UNTERkante
usb_y_offset = 0;   // Versatz der Buchse aus der Board-Mitte (quer)
board_gap_usb = 1.0; // Abstand Board-Kante -> USB-Innenwand (klein halten,
                     // sonst erreicht der Stecker die Buchse nicht!)

// USB-WECHSELADAPTER (Variante B): Das Unterteil bekommt nur ein
// großzügiges, nach oben offenes Fenster. Ein kleiner Einsatz trägt
// das präzise USB-Loch, wird von oben ins Fenster geschoben (sein
// U-Profil umgreift die Wandkanten links/rechts/unten) und vom
// aufgeschraubten Deckel verriegelt. Bei Maßfehlern wird nur der
// Einsatz neu gedruckt, nicht das Unterteil!
usb_win_w     = 16.0; // Fensterbreite im Unterteil
usb_win_bot   = 2.7;  // Fensterunterkante unter der Loch-Unterkante
usb_ins_clear = 0.2;  // Spiel Einsatz <-> Fenster (je Seite)
usb_flange    = 1.5;  // Überstand der Außenblende über das Fenster
usb_flange_t  = 1.0;  // Dicke der Außenblende
usb_rib_t     = 0.6;  // Dicke der Innenrippen (MAX ~0,8 – das Board
                      // liegt nur board_gap_usb hinter der Wand!)

// Akku (Makerfocus 3000mAh – unbedingt messen, Chargen variieren!)
bat_l = 60.0;
bat_w = 36.0;
bat_t = 10.0;
bat_clearance = 1.5;  // Luft fürs Aufquellen

// Schiebeschalter (Aussparung in der Seitenwand, 0-setzen wenn keiner)
switch_w = 0;       // deaktiviert – Gerät nutzt Deep Sleep statt Schalter
switch_h = 4.0;

// Gehäuse
wall = 1.2;         // Wandstärke
floor_t = 1.2;      // Bodenstärke
lid_t = 1.8;        // Deckelstärke (genug Fleisch für die Kegelsenkungen)
fit_gap = 0.4;      // Spielpassung zwischen Teilen
corner_r = 3.5;     // Eckenradius außen
separator = true;   // Trennboden = separat gedrucktes, entnehmbares Einlegeteil
sep_t = 1.0;        // Dicke der Einlegeplatte

// Schrauben (M2.5 selbstschneidend in Kunststoff)
screw_d = 2.2;      // Kernloch im Unterteil
screw_head_d = 5.0; // konische Senkung für M2.5-Senkkopf (DIN 965 ~4,7 mm)
post_d = 7.0;       // Durchmesser Schraubdome

// Extras
lanyard = false;     // Öse für Schlüsselband
led_hole = false;    // 1mm Loch für Lade-LED (Position anpassen!)
led_x = 8.0;        // LED-Position von USB-Kante
led_y = 5.0;        // LED-Position von Längskante

// Flex-Taster im Deckel: gedruckte Federzungen mit Stößel, die die
// Taster (RST/PRG) auf dem Board direkt drücken. Positionen = Mitte
// der Taster auf dem Board – unbedingt nachmessen!
// Format pro Taster: [x von USB-Kante, y, stoessel_rueckzug]
//   stoessel_rueckzug = zusätzlicher Luftspalt (mm) über plunger_gap;
//   größer = Stößel kürzer = löst NICHT versehentlich aus.
btn_holes = [ [4.5, 4.0,            1.0],    // feste-C-Balken-Seite: -1 mm
              [4.5, board_w-4.0,    2.0] ];  // Trapez-/Schieber-Seite (RST): -2 mm
btn_height = 2.0;   // Höhe der Taster über der Platine (messen!)
tongue_w   = 3.5;   // Breite der Federzunge (= Breite des Stößel-Quaders)
tongue_l   = 14.0;  // Länge der Federzunge (Richtung Board-Mitte)
tongue_t   = 1.4;   // Restdicke der Zunge (dünner = leichter zu drücken)
slot_w     = 0.8;   // Breite der U-Schlitze (druckbar ab ~1 mm)
plunger_gap = 0.2;  // Luft zwischen Stößel und Taster in Ruhe
board_head = 6.0;   // Luft zwischen Board-Oberkante und Deckelunterseite

// Board-Halterung am Deckel (nur an den LANGSEITEN, wegen Bauteilen):
guide_t     = 1.6;  // Dicke der festen Schienenwand
guide_clear = 0.2;  // seitliches Spiel Board-Kante <-> Schienenwand
rail_clear  = 0.3;  // Höhenspiel in der Nut (zusätzlich zur Platinendicke;
                    // etwas größer, damit das Board sich einkippen lässt)
rail_lip    = 1.5;  // wie weit die Lippe unter die Board-Kante greift
                    // (klein halten – an der Unterseite liegen Lötstellen!)
rail_lip_fest = rail_lip + 1.5; // feste C-Schiene greift 1,5 mm tiefer
                    // unter die Platine (sicherer Halt beim Einhängen)
guide_extra = 0.5;  // feste Schienenwand zusätzlich Richtung Platine
                    // verdickt. ACHTUNG: verengt den Sitz unter Nennmaß
                    // (board_w) und gleicht so Druckspiel aus. Sitzt das
                    // Board zu stramm/gar nicht: Wert verkleinern!
lip_t       = 1.6;  // Dicke der Lippe unter dem Board
board_shift = -3.5; // Board aus der Mitte zur festen Schiene verschoben,
                    // schafft Platz für die Schwalbenschwanz-Führung
under_clear = pins_below + 0.5;  // Luft unter der Platine im Gehäuse

// Verstärkung des End-Anschlags: Stützrippen (Gussets) am Fuß des
// dünnen Anschlag-Wändchens, auf der ANTENNEN-Seite (bauteilfrei) –
// gegen Abbrechen an der Layer-Fußkante.
endstop_ribs    = 3;   // Anzahl Stützrippen (0 = keine)
endstop_rib_t   = 2.0; // Dicke jeder Rippe (in Board-Querrichtung y)
endstop_rib_out = 4.5; // wie weit die Rippe vom Wändchen wegläuft (+x)
endstop_rib_up  = 5.5; // Höhe der Rippe an der Wand (von max. ~7,6 mm)
endstop_embed   = 0.6; // Einbindung der Rippe in die Wand (Verschmelzung)

// Schwalbenschwanz-Führung für die Schiebeschiene:
dove_neck  = 3.2;   // Trapez-Breite am Deckel (schmale Seite)
dove_head  = 5.2;   // Trapez-Breite am freien Ende (breite Seite)
dove_h     = 2.4;   // Höhe des Trapezprofils
dove_clear = 0.15;  // Spiel der Nut im Schieber – straff! Klemmt der
                    // Schieber nach dem Druck: auf 0,2-0,25 erhöhen,
                    // wackelt er: auf 0,1 reduzieren
slider_wall = 1.6;  // Wandstärke des Schiebers neben der Nut
cap_l      = 3.0;   // Länge der geschlossenen Endkappe (= Anschlag)

$fn = 48;

// ---------- ABGELEITETE MASSE ----------
post_in = post_d - wall;            // Überstand der Schraubdome in den Innenraum
// Innenraum so groß, dass der Akku zwischen den Eckdomen Platz hat:
inner_l = max(board_l, bat_l + bat_clearance + 2*post_in) + 2;
inner_w = max(board_w, bat_w + bat_clearance + 2*post_in) + 2;
bat_bay_h  = bat_t + bat_clearance;                          // Akkufach-Höhe
sep_eff    = separator ? sep_t : 0;                          // wirksame Dicke
board_z    = floor_t + bat_bay_h + sep_eff + under_clear;    // OK Platine
inner_h    = bat_bay_h + sep_eff + under_clear + board_t + board_head;
outer_l = inner_l + 2*wall;
outer_w = inner_w + 2*wall;
outer_h = floor_t + inner_h;                                  // Unterteil-Höhe

// Board-Lage (für Deckel UND Unterteil – USB-Loch folgt dem Board!)
board_x = wall + board_gap_usb;                       // Ursprung x (USB-Wand)
board_y = wall + (inner_w-board_w)/2 + board_shift;   // Ursprung y
rail_h  = board_head + board_t + rail_clear + lip_t;  // Höhe Schiene/Schieber
slider_w = dove_head + 2*slider_wall;                 // Breite des Schiebers
slider_y = board_y + board_w + guide_clear;           // Innenfläche Schieber
dove_yc  = slider_y + slider_w/2;                     // Mitte Schwalbenschwanz
// USB-Fenster (Deckel wird um die Längsachse gewendet -> y gespiegelt):
usb_cy    = outer_w - (board_y + board_w/2 + usb_y_offset); // Fenster-Mitte
win_bot_z = board_z + usb_z_lift - usb_win_bot;       // Fensterunterkante
win_h     = outer_h - win_bot_z;                      // Fensterhöhe (offen n. oben)

echo("Außenmaße (B x L x H ohne Deckel):", outer_w, outer_l, outer_h + lid_t);
echo("Schieber (L x B x H):", board_l + cap_l, slider_w + rail_lip, rail_h);
echo("USB-Einsatz Körper (B x H):", usb_win_w - 2*usb_ins_clear, win_h - 0.3);

// ---------- HILFSMODULE ----------
module rbox(l, w, h, r) {           // Quader mit runden Ecken
    hull() for (x=[r, l-r], y=[r, w-r])
        translate([x, y, 0]) cylinder(h=h, r=r);
}

module screw_posts(hole_d, h) {
    for (x=[wall+post_d/2, outer_l-wall-post_d/2],
         y=[wall+post_d/2, outer_w-wall-post_d/2])
        translate([x, y, 0]) difference() {
            cylinder(d=post_d, h=h);
            translate([0,0,2]) cylinder(d=hole_d, h=h);
        }
}

// Trapezprofil entlang x; w_top liegt bei z=0 (am Deckel), w_bot bei z=h
module trapez_x(len, w_top, w_bot, h) {
    rotate([90,0,90])
        linear_extrude(height=len)
            polygon([[-w_top/2,0],[w_top/2,0],[w_bot/2,h],[-w_bot/2,h]]);
}

// ---------- UNTERTEIL ----------
module bottom() {
    difference() {
        union() {
            // Außenschale
            difference() {
                rbox(outer_l, outer_w, outer_h, corner_r);
                translate([wall, wall, floor_t])
                    rbox(inner_l, inner_w, outer_h, corner_r-wall/2);
            }
            // Auflagenocken für die entnehmbare Trennboden-Platte
            if (separator)
                for (x = [wall + inner_l*0.25, wall + inner_l*0.75 - 8],
                     y = [wall, wall + inner_w - 2])
                    translate([x, y, floor_t + bat_bay_h - 2])
                        cube([8, 2, 2]);
            // Schraubdome
            screw_posts(screw_d, outer_h);
            // Trageöse
            if (lanyard)
                translate([outer_l, outer_w/2, 0]) difference() {
                    cylinder(d=10, h=floor_t+3);
                    translate([0,0,-1]) cylinder(d=4.5, h=floor_t+5);
                }
        }
        // USB-Fenster (nach oben offen) für den Wechseladapter.
        // ACHTUNG: Der Deckel wird zum Aufsetzen um die LÄNGSachse
        // gewendet -> Fensterposition gespiegelt zur Board-Lage.
        translate([-1, usb_cy - usb_win_w/2, win_bot_z])
            cube([wall+2, usb_win_w, outer_h]);
        // Schiebeschalter-Aussparung (Längsseite)
        if (switch_w > 0)
            translate([wall + 8, -1, floor_t + bat_bay_h/2 - switch_h/2])
                cube([switch_w, wall+2, switch_h]);
    }
}

// ---------- DECKEL ----------
// U-Schlitze + Ausdünnung für eine Federzunge (Flex-Taster)
module flex_slots(px, py) {
    tip = px - tongue_w/2 - 1.0;             // Spitze der Zunge
    // zwei Längsschlitze
    for (s = [-1, 1])
        translate([tip, py + s*tongue_w/2 - slot_w/2, -1])
            cube([tongue_l, slot_w, lid_t+2]);
    // Querschlitz an der Spitze
    translate([tip - slot_w, py - tongue_w/2 - slot_w/2, -1])
        cube([slot_w, tongue_w + slot_w, lid_t+2]);
    // Zunge von innen ausdünnen -> leichter zu drücken
    translate([tip, py - tongue_w/2, tongue_t])
        cube([tongue_l, tongue_w, lid_t]);
    // flache Mulde außen als fühlbare Markierung
    //translate([px, py, -0.01]) cylinder(d=tongue_w+2.5, h=0.4);
}

module lid() {
    bx = board_x;
    by = board_y;
    union() {
        difference() {
            union() {
                rbox(outer_l, outer_w, lid_t, corner_r);
                // Innenlippe für die Passung
                translate([wall+fit_gap, wall+fit_gap, lid_t])
                    difference() {
                        rbox(inner_l-2*fit_gap, inner_w-2*fit_gap, 2.5,
                             corner_r-wall/2);
                        translate([1.6, 1.6, -1])
                            rbox(inner_l-2*fit_gap-3.2,
                                 inner_w-2*fit_gap-3.2, 5, 1);
                    }
            }
            // Display-Fenster (1mm vertieft = Rand schützt das OLED)
            translate([bx+oled_x, by+oled_y, -1])
                cube([oled_w, oled_h, lid_t+4]);
            translate([bx+oled_x-1.5, by+oled_y-1.5, lid_t-1])
                cube([oled_w+3, oled_h+3, 2]);   // Vertiefung außen
            // Flex-Taster: U-Schlitze und Ausdünnung
            for (p = btn_holes) flex_slots(bx+p[0], by+p[1]);
            // Lade-LED-Loch
            if (led_hole)
                translate([bx+led_x, by+led_y, -1]) cylinder(d=1.2, h=lid_t+4);
            // Schraubenlöcher mit konischer Senkung (M2.5 Senkkopf, 90°)
            for (x=[wall+post_d/2, outer_l-wall-post_d/2],
                 y=[wall+post_d/2, outer_w-wall-post_d/2])
                translate([x, y, 0]) {
                    translate([0,0,-1]) cylinder(d=2.8, h=lid_t+2);
                    translate([0,0,-0.01])
                        cylinder(d1=screw_head_d, d2=2.6,
                                 h=(screw_head_d-2.6)/2);
                }
            // Freischnitte in der Innenlippe an den vier Schraubdomen –
            // sonst kollidiert die Lippe beim Aufsetzen mit den Domen!
            for (x=[wall+post_d/2, outer_l-wall-post_d/2],
                 y=[wall+post_d/2, outer_w-wall-post_d/2])
                translate([x, y, lid_t - 0.01])
                    cylinder(d=post_d + 0.6, h=6);
            // Durchlass in der Innenlippe über dem USB-Fenster -> muss
            // auch die Innenrippen des Adapters freistellen (Fenster +
            // Rippenüberstand + Spiel); der Deckel drückt auf die
            // Adapter-Oberkante und verriegelt ihn damit im Fenster
            translate([-1,
                       board_y + board_w/2 + usb_y_offset
                           - usb_win_w/2 - 2.2,
                       lid_t])
                cube([wall + 5, usb_win_w + 4.4, 6]);
            // Durchlass in der Innenlippe für den Schieber (beide kurzen
            // Seiten): von der Antennen-Seite wird er aufgeschoben, an der
            // USB-Seite endet er bündig in der Lippen-Ebene.
            for (gx = [-1, outer_l - wall - 4])
                translate([gx, slider_y - 0.4, lid_t])
                    cube([wall + 5, slider_w + 0.8, 6]);
        }
        // Flex-Taster: Stößel als Quader in Zungenbreite
        // Länge: endet (plunger_gap + Rückzug p[2]) über dem Taster in
        // Ruhestellung. Rückzug pro Taster verhindert ungewolltes Auslösen.
        for (p = btn_holes)
            translate([bx+p[0] - tongue_w/2, by+p[1] - tongue_w/2,
                       tongue_t - 0.01])
                cube([tongue_w, tongue_w,
                      (lid_t - tongue_t) + board_head
                      - btn_height - plunger_gap
                      - (len(p) > 2 ? p[2] : 0)]);
        // FESTE C-Schiene (Langseite bei y = board_y):
        // Wand + Lippe unter der Board-Kante. Hier wird das Board
        // schräg eingehängt. Wand ist um guide_extra Richtung Platine
        // verdickt (Spielausgleich), die Lippe greift rail_lip_fest
        // über die Wandfläche hinaus unter die Platine.
        translate([bx, by - guide_clear - guide_t, lid_t])
            cube([board_l, guide_t + guide_extra, rail_h]);
        translate([bx, by - guide_clear - guide_t,
                   lid_t + board_head + board_t + rail_clear])
            cube([board_l, guide_t + guide_extra + rail_lip_fest, lip_t]);
        // SCHWALBENSCHWANZ-Führung für die Schiebeschiene (zweite
        // Langseite). Trapez: schmal am Deckel, breit am freien Ende
        // -> der Schieber kann nicht abfallen.
        translate([bx, dove_yc, lid_t])
            trapez_x(board_l, dove_neck, dove_head, dove_h);
        // Rastnase nahe der USB-Seite: der Schieber klickt am Ende
        // seines Weges darüber (Gegenstück = Mulde in der Schieber-Nut).
        translate([bx + 4, dove_yc - dove_head/2, lid_t + dove_h])
            rotate([-90,0,0]) cylinder(d=1.0, h=dove_head);
        // Anschlag an der Antennen-Stirnseite (Board-Position längs).
        // Position prüfen: darf Antennenanschluss/Pigtail nicht treffen –
        // ggf. schmaler machen oder versetzen!
        translate([bx + board_l + guide_clear, by + board_w/2 - 6, lid_t])
            cube([guide_t, 12, board_head + board_t]);
        // Stützrippen (Gussets) am Fuß des Anschlags, auf der ANTENNEN-
        // Seite (+x, bauteilfrei) -> Wändchen bricht nicht mehr ab.
        if (endstop_ribs > 0) {
            es_x = bx + board_l + guide_clear + guide_t;  // Antennen-Face
            es_y0 = by + board_w/2 - 6;                    // Anschlag-Start y
            for (i = [0 : endstop_ribs-1]) {
                ry = (endstop_ribs == 1) ? es_y0 + 6
                     : es_y0 + 1.5 + i*(12 - 3)/(endstop_ribs - 1);
                hull() {
                    // dünner vertikaler Steg an der Wand (in Wand eingebettet)
                    translate([es_x - endstop_embed, ry - endstop_rib_t/2,
                               lid_t])
                        cube([0.8, endstop_rib_t, endstop_rib_up]);
                    // flacher Steg auf dem Deckelboden, läuft nach +x aus
                    translate([es_x - endstop_embed, ry - endstop_rib_t/2,
                               lid_t])
                        cube([endstop_rib_out, endstop_rib_t, 0.8]);
                }
            }
        }
    }
}

// ---------- SCHIEBESCHIENE (separates Druckteil) ----------
// Wird von der Antennen-Seite auf den Schwalbenschwanz geschoben und
// verriegelt die zweite Board-Langskante. Nut ist am USB-Ende offen
// und am anderen Ende geschlossen (Endkappe = Anschlag).
// Druck: am besten stehend auf dem USB-Ende (mit Brim), alternativ
// mit der Nut nach oben liegend.
module slider_rail() {
    sl_l = board_l + cap_l;
    difference() {
        union() {
            cube([sl_l, slider_w, rail_h]);          // Grundkörper
            // Lippe unter der Board-Kante (zeigt zum Board, y < 0)
            translate([0, -rail_lip, board_head + board_t + rail_clear])
                cube([sl_l, rail_lip + 0.01, lip_t]);
        }
        // Schwalbenschwanz-Nut: offen bei x=0 (USB-Ende),
        // blind ab x = board_l (Endkappe)
        translate([-0.01, slider_w/2, -0.01])
            trapez_x(board_l + 0.01,
                     dove_neck + 2*dove_clear,
                     dove_head + 2*dove_clear,
                     dove_h + dove_clear + 0.01);
        // Rast-Mulde (Gegenstück zur Nase auf dem Schwalbenschwanz)
        translate([4, slider_w/2 - dove_head/2 - dove_clear,
                   dove_h + dove_clear])
            rotate([-90,0,0]) cylinder(d=1.4, h=dove_head + 2*dove_clear);
        // Einführfase an der Nut-Öffnung
        translate([-0.01, slider_w/2, -0.01])
            trapez_x(1.0,
                     dove_neck + 2*dove_clear + 1.5,
                     dove_head + 2*dove_clear + 1.5,
                     dove_h + dove_clear + 0.01);
    }
}

// ---------- USB-WECHSELADAPTER (separates Druckteil) ----------
// Trägt das präzise USB-Loch. Das U-Profil (Blende außen, Rippen
// innen) umgreift die Fensterkanten links, rechts und unten. Von oben
// ins Fenster schieben; der aufgeschraubte Deckel verriegelt ihn.
// Lokale Koordinaten: x = Wanddicke (0 = Wand-Außenfläche),
// z = 0 an der Einsatz-Unterkante.
module usb_adapter() {
    bw     = usb_win_w - 2*usb_ins_clear;   // Körperbreite
    bh     = win_h - 0.3;                   // Körperhöhe (oben wandbündig)
    body_t = wall + 0.15;                   // +0,15 = Gleitspiel im U-Profil
    hole_z = usb_win_bot - 0.3;             // Loch-Unterkante lokal
    difference() {
        union() {
            // Außenblende (überdeckt das Fenster links/rechts/unten)
            translate([-usb_flange_t, -usb_win_w/2 - usb_flange,
                       -0.3 - usb_flange])
                cube([usb_flange_t, usb_win_w + 2*usb_flange,
                      bh + 0.3 + usb_flange]);
            // Körper (füllt das Fenster, innen 0,15 mm überstehend)
            translate([0, -bw/2, 0]) cube([body_t, bw, bh]);
            // Innenrippen seitlich (hintergreifen die Wand)
            for (sy = [-1, 1])
                translate([body_t,
                           sy < 0 ? -usb_win_w/2 - 1.6 : bw/2 - 0.7, 0])
                    cube([usb_rib_t,
                          (usb_win_w/2 + 1.6) - (bw/2 - 0.7), bh]);
            // Innenrippe unten
            translate([body_t, -bw/2, -0.3 - 1.5])
                cube([usb_rib_t, bw, 1.5 + 0.3 + 0.7]);
            // Klemmpunkte: leichter Reibsitz/"Klick" beim Einschieben
            for (sy = [-1, 1])
                translate([body_t, sy*(usb_win_w/2 + 0.8), bh*0.6])
                    sphere(d=0.6);
        }
        // Blenden-Tasche, damit dicke Stecker-Gehäuse näher herankommen
        translate([-usb_flange_t - 0.01, -(usb_w + 2.5)/2, hole_z - 1.25])
            cube([usb_flange_t + 0.02, usb_w + 2.5, usb_h + 2.5]);
        // präzises USB-Loch (DAS Maß, das man iteriert)
        translate([-usb_flange_t - 0.01, -usb_w/2, hole_z])
            cube([usb_flange_t + body_t + 0.02, usb_w, usb_h]);
    }
}

// ---------- TRENNBODEN (entnehmbare Einlegeplatte) ----------
// Nur noch glatte Abdeckung über dem Akku – das Board hängt am Deckel.
module separator_plate() {
    pl = inner_l - 0.6;   // 0,3 mm Spiel pro Seite
    pw = inner_w - 0.6;
    difference() {
        rbox(pl, pw, sep_t, 1.5);
        // Eckausschnitte für die Schraubdome
        for (x = [post_d/2 - 0.3, pl - post_d/2 + 0.3],
             y = [post_d/2 - 0.3, pw - post_d/2 + 0.3])
            translate([x, y, -1]) cylinder(d=post_d + 1, h=sep_t + 2);
        // Durchlass für das Akkukabel (am Ende gegenüber USB)
        translate([pl - 11, pw/2 - 6, -1]) cube([12, 12, sep_t + 2]);
        // Fingerloch zum Herausnehmen
        translate([pl/2, pw/2, -1]) cylinder(d=10, h=sep_t + 2);
    }
}

// ---------- AUSGABE ----------
// part wählen, F6 rendern, dann STL exportieren -> nur dieses Teil im STL.
// Möglich: "all", "bottom", "lid", "sep", "slider", "usb"
part = "all";

if (part == "all" || part == "bottom")
    bottom();
if (part == "all" || part == "lid")
    translate(part == "all" ? [0, outer_w + 8, 0] : [0, 0, 0])
        lid();   // Innenseite oben = richtige Druckorientierung
if (separator && (part == "all" || part == "sep"))
    translate(part == "all" ? [0, -(inner_w + 8), 0] : [0, 0, 0])
        separator_plate();
if (part == "all" || part == "slider")
    translate(part == "all" ? [0, 2*outer_w + 16, 0] : [0, 0, 0])
        slider_rail();
if (part == "all" || part == "usb")
    // liegend, Blende nach oben (Rippen auf dem Bett, Körper als
    // kurze Brücke zwischen den Rippen) = beste Druckorientierung
    translate(part == "all" ? [outer_l + 14, 2*outer_w + 16, 0] : [0, 0, 0])
        translate([0, 0, wall + 0.15 + usb_rib_t])
            rotate([0, 90, 0])
                usb_adapter();
