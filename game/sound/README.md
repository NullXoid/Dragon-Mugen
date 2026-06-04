# Sound

This folder is for shared/global sound assets.

Character-owned sounds belong in `chars/<character>/`. Shared common and fight sounds currently load from `game/data/common.snd` and `game/data/fight.snd`, matching the imported M.U.G.E.N layout. Stage music can be referenced from stage definitions. Global menu, UI, announcer, and system sounds may live here when they are not part of the M.U.G.E.N `data` files.

Current runtime behavior:

- Selected character SND loads from that character's DEF `[Files] sound` entry.
- Common/fight fallback sounds load from `game/data`.
- Stage/system BGM entries are content metadata for now; the current runtime only mixes SFX voices and does not start background music playback.
- This folder remains available for future shared project audio that is not already owned by M.U.G.E.N `data` files.
