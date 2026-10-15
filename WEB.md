# Play the browser version

The existing C/C++ game is compiled to WebAssembly with raylib 5.5. The web version includes the referee and uses the keyboard for the home team. Desktop gamepad defaults are unchanged.

## Deploy to Vercel

The ready-to-deploy files are committed in `dist/`; Vercel does not need a C compiler or Emscripten. `vercel.json` selects that directory and disables the build and installation steps.

From a checkout containing this version, run:

```sh
npx vercel
```

Sign in to your Vercel account and follow the project prompts, using the repository root as the project directory. The command creates a preview deployment. To publish it as production later, use `npx vercel --prod`.

Alternatively import the repository in Vercel with this web branch selected. Use framework **Other**, no build or installation command, and output directory **dist**. If Vercel initially imports the default branch, change the project's production branch to the web branch and redeploy it.

No database, secret, backend, SharedArrayBuffer or cross-origin isolation headers are required. Serve the files over HTTP(S), not by opening index.html as a local file.

## Controls

Before starting, choose one of five formations for the red team: **5-4-1**, **5-3-2**, **4-4-2**, **3-4-3**, or **3-2-3-2**. The opponent uses 4-4-2. All five formations field ten outfield players plus the goalkeeper. The 3-2-3-2 replaces the initially requested 3-2-3-1 so both sides have eleven players.

The formation changes starting positions and role-based player archetypes, possession support positions, defensive compactness, attacking width, defensive line height and pressing range. Defensive 5-4-1 and 5-3-2 hold lower lines; 5-3-2 wing-backs advance on the ball. Balanced 4-4-2 supports both flanks with two strikers. Attacking 3-4-3 pushes the wide players forward, while 3-2-3-2 adds two holding midfielders behind three support players and two strikers. Off-ball players shift toward the ball side and drop into their formation when possession changes.

Native builds also accept a formation code as a command-line argument, for example `./build/macos-manual/thegame 532`.

| Key | Action |
| --- | --- |
| Arrow keys | Move selected player |
| A | Hold and release to shoot; press for a header on a high ball |
| S | Automatic pass / standing tackle |
| D | High pass or cross / sliding tackle |
| Q | Switch player |
| E | Slow down / close control |
| W, 1 | Existing left/right shot and pass modifiers |

Click the canvas to focus the controls. The page includes fullscreen and restart buttons. Keyboard controls require a desktop browser with WebGL 2. Native video export and the memory-intensive debug replay are disabled on the web; the match and referee use the same gameplay systems as desktop.

## Rebuild after code or asset changes

Use Emscripten **3.1.69** and activate its `emsdk_env.sh`, then:

```sh
bash scripts/build_web.sh
```

The script obtains raylib 5.5 at commit `c1ab645ca298a2801097931d1079b10ff7eb9df8`, builds a separate WebGL 2 library with manual frame control, and writes the static bundle to `dist/`. Commit the regenerated `dist/` files with source changes before redeploying. Native build artifacts are kept separate.

With Docker installed, the same build can run without a local Emscripten installation:

```sh
docker run --rm -v "$PWD:/src" -w /src emscripten/emsdk:3.1.69 bash scripts/build_web.sh
```

## Browser smoke test

```sh
npm ci
npx playwright install chromium
npm run test:web
```

To use an already installed Chromium instead of downloading one, set `PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH` to its executable. The test serves the actual deployment files temporarily and checks engine startup, shader errors, referee creation, keyboard shot/pass/cross input reaching the game, and the disabled replay shortcut.

The current bundle was exercised in Chromium with software WebGL. Safari and Firefox need a separate device check; publication on Vercel is a separate step from the local browser validation.


## Movement and restarts

Players keep their formation lanes while making role-dependent supporting movements:
forwards check and run, wide players advance on the ball side, midfielders offer
angled support, and defenders track opponents inside their zone. Forwards move
into separate central lanes when the ball reaches a crossing position.

Kickoffs, throw-ins, corners, goal kicks, direct free kicks and penalties now have
a two-second preparation phase and an automatic delivery. The restart selects a
taker on the awarded team and positions the other players, including a three-man
free-kick wall and players outside the penalty spot at penalties. Challenges that
hit an opponent without playing the ball award a free kick, or a penalty inside
the defending team's penalty area.

This is the initial restart implementation: deliveries are automatic and throw-ins
use the existing passing physics. Dedicated throw-in animations, manual restart
aiming, indirect free kicks, offside, dropped balls, advantage, cards and penalty
shootouts are not implemented.
