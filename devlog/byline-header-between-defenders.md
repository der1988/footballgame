# Byline cross, header between two defenders, goal

[Download the video](https://github.com/der1988/footballgame/raw/refs/heads/devlog-media/devlog/byline-header-between-defenders-60fps.mp4)

7.5 seconds, 1280 x 720, 60 fps, 450 rendered frames. Silent H.264 MP4 with fast start. File size: 564,417 bytes (approximately 564 KB).

Prepared in-engine scene with scripted player inputs and initial positions. The winger dribbles down the right flank to the byline. The attacker is flanked by two nearby opponents, heads the cross and scores past the diving keeper. Original heading collision, ball physics and goal detection execute the finish.

Verified during the exported take (timestamps relative to the start of the video):

| Event | Time | Check |
| --- | --- | --- |
| Byline cross | 4.723 s | Ball x=850.00, y=207.80; byline y=207.00, within 0.80 game pixels |
| Header | 5.725 s | Attacker at (550,310); opponents at (530,310) and (570,310), one on each side, each 20 game pixels away |
| Goal | 6.123 s | Goal detection fired; last deliberate ball play is BPT_HEADER |

The scoring action takes over 6 seconds, followed by the ball settling in the net. All 450 frames were rendered directly from deterministic 1/480-second simulation steps, eight steps per video frame. The final MP4 was decoded end to end without errors.

Suggested caption:

> Down to the byline, a cross into the box, and a header between two defenders. A little finishing setup from my football project. #gamedev #raylib
