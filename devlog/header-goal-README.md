# Cross, header, goal

[Download the MP4](https://github.com/der1988/footballgame/raw/refs/heads/devlog-media/devlog/header-goal-60fps.mp4)

- 3 seconds, 1280 x 720, 60 fps; 180 rendered frames.
- H.264, yuv420p, fast-start MP4, silent.
- File size: 195,569 bytes (approximately 196 KB).
- A prepared in-engine showcase: arranged starting positions, an incoming cross and scripted player controls. Original heading collision, ball physics, keeper AI and goal detection execute the finish.
- Deterministic capture: eight 1/480-second simulation steps per rendered frame, exported directly to the encoder, avoiding real-time screen-capture frame loss.
- Verified by the engine: header at 0.823 seconds; goal at 1.265 seconds; last deliberate ball play at scoring is BPT_HEADER.

Suggested devlog caption:

> A quick heading test in my football game: cross in, jump, far corner. Captured this little setup at 60 fps. Still a work in progress, but this finish made me smile. #gamedev #raylib

Video description: A blue-shirted player sends the ball across the penalty area. A teammate jumps to head it into the far side of the goal, beyond the diving goalkeeper, with two red defenders nearby.
