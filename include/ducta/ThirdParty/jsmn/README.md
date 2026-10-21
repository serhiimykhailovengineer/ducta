# jsmn

Minimalistic JSON tokenizer used by `ducta/Configuration/Json.hpp`, vendored unchanged from
https://github.com/zserge/jsmn at commit `25647e692c7906b96ffd2b05ca54c097948e879c`
(MIT license, see `LICENSE` in this folder).

Ducta includes it with `JSMN_STATIC` and `JSMN_STRICT`, so its functions are local to the
translation unit and do not clash with another copy of jsmn in the application.
