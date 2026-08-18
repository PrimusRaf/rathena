# Gildenemblem als MVP- und Miniboss-Marke

**Nicht angewandt.** Das hier ist der Vorschlag zum Durchlesen. Nichts im
Quelltext ist angefasst.

## Warum das ueberhaupt geht

Der Client zeichnet Embleme sehr wohl an Monstern - der Beweis steht in
diesem Quelltext: **WoE-Waechter sind ganz normale `BL_MOB`**, und die
tragen im Krieg sichtbar das Emblem ihrer Gilde. Es fehlt also keine
Faehigkeit im Client, es fehlt nur der Weg, einem gewoehnlichen Monster
eine Gilde zuzuweisen.

Zwei Stellen sperren das heute:

`src/map/status.cpp`, Zeile 9187 und 9237 - beide Funktionen kennen fuer
`BL_MOB` genau zwei Faelle:

```cpp
	case BL_MOB:
		{
			const mob_data *md = static_cast<const mob_data*>(bl);
			if (md->guardian_data)          // Waechter
				return md->guardian_data->guild_id;
			if (md->special_state.ai){      // Beschwoerung des Alchemisten
				if(map_session_data* msd = map_id2sd(md->master_id); msd != nullptr)
					return msd->status.guild_id;
			}
		}
		break;
```

Und `setunitdata` hat kein Feld dafuer: die Liste reicht von `UMOB_SIZE`
bis `UMOB_DAMAGETAKEN`, eine Gilde ist nicht dabei.

Das Spawn-Paket dagegen traegt die Felder laengst mit -
`packets_struct.hpp`, Zeile 727:

```cpp
	uint32 GUID;
	int16 GEmblemVer;
```

gefuellt aus `clif.cpp` Zeile 1048 und 1080:

```cpp
	int32 g_id = status_get_guild_id( bl );
	...
	p.GUID = g_id;
	p.GEmblemVer = status_get_emblem_id( bl );
```

## Der Eingriff

Fuenf Dateien, rund vierzig Zeilen. Kein bestehendes Verhalten aendert
sich: ohne die neue Einstellung bleibt `fake_guild_id` null und alles
laeuft wie bisher.

### 1. `src/map/mob.hpp` - zwei Felder in `mob_data`

Neben `struct guardian_data* guardian_data;` (Zeile 364):

```cpp
	// Raffas_RO: Gilde nur zur Anzeige. Das Monster gehoert keiner
	// Gilde an, es fuehrt lediglich deren Emblem - sonst nichts:
	// keine Beziehungen, keine Kriegsziele, kein Zugriff auf
	// Gildenlager. Beides null heisst "wie vorher".
	int32 fake_guild_id;
	int32 fake_emblem_id;
```

### 2. `src/map/status.cpp` - beide Abfragen oeffnen

In `status_get_guild_id`, im `BL_MOB`-Zweig **vor** der
Waechter-Abfrage:

```cpp
			if (md->fake_guild_id)
				return md->fake_guild_id;
```

In `status_get_emblem_id` an derselben Stelle:

```cpp
			if (md->fake_emblem_id)
				return md->fake_emblem_id;
```

Vorher, nicht nachher: ein Waechter soll weiterhin das Emblem seiner
echten Gilde tragen, und der ist bei uns ohnehin nie markiert.

### 3. `src/map/mob.cpp` - beim Erscheinen setzen

Das ist der Punkt, an dem es ohne Skript und ohne Zeitgeber auskommt.
In `mob_spawn()` (Zeile 1117), nachdem `md->db` steht:

```cpp
	// Raffas_RO: MVPs und Minibosse bekommen ihre Marke beim
	// Erscheinen. Hier und nicht per Skript, weil es sonst jemanden
	// braeuchte, der alle Karten abklappert - mob_spawn laeuft
	// dagegen fuer jedes Monster, egal wer es gesetzt hat.
	if (status_has_mode(&md->status, MD_MVP))
		md->fake_guild_id = md->fake_emblem_id = battle_config.mvp_emblem_guild;
	else if (md->status.class_ == CLASS_BOSS)
		md->fake_guild_id = md->fake_emblem_id = battle_config.miniboss_emblem_guild;
```

`MD_MVP` ist der echte MVP-Modus, `CLASS_BOSS` der Miniboss - das ist
dieselbe Unterscheidung, die auch die Datenbank trifft.

### 4. `conf/battle/monster.conf` - zwei Schalter

```
// Gilden-ID, deren Emblem MVPs tragen. 0 = aus.
mvp_emblem_guild: 0

// Dasselbe fuer Minibosse. 0 = aus.
miniboss_emblem_guild: 0
```

Dazu je ein Eintrag in `src/map/battle.cpp` in der `battle_data`-Tabelle
(Vorgabe 0) und im passenden `struct Battle_Config`.

### 5. Zwei Gilden mit den Emblemen

Emblembilder liegen nicht als Datei im Client, sondern als Blob in der
Gildentabelle - sie werden im Spiel hochgeladen. Also einmalig:

1. zwei Gilden gruenden, etwa `MVP` und `Mini`
2. je ein 24 x 24 grosses Emblem hochladen
3. `SELECT guild_id, name FROM guild;` und die beiden IDs in die
   Konfiguration eintragen

Die Embleme selbst muessen wir nicht von irgendwo abgreifen - die
Sprite-Strecke unter `tools/sprite` erzeugt bereits BMPs, ein 24 x 24
Abzeichen ist dieselbe Uebung wie ein Inventarsymbol.

## Was daran unsicher bleibt

**Ich habe es nicht laufen sehen.** Der Beweis, dass der Client Embleme
an Monstern zeichnet, sind die WoE-Waechter - das ist ein starkes
Indiz, aber kein Test. Moeglich ist, dass der Client das Emblem nur
anfordert, wenn das Objekt zusaetzlich einen Gildennamen im
Namenspaket hat; dann muesste in `clif.cpp` Zeile 10028 der
Waechter-Zweig entsprechend erweitert werden.

**Der erste Versuch sollte deshalb klein sein:** einen einzelnen MVP von
Hand markieren und nachsehen, ob das Emblem erscheint. Erst wenn das
steht, lohnt der Rest.

## Was der Patch NICHT tut

- Er gibt dem Monster keine Gilde im Sinne des Spiels. Kein Krieg,
  keine Beziehungen, kein Lager - nur die zwei Zahlen im Spawn-Paket.
- Er beruehrt Waechter und Alchemisten-Beschwoerungen nicht.
- Ohne die Konfiguration aendert sich gar nichts.
