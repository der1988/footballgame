-- Aseprite batch script.
-- Exports each leaf (non-group) layer of the active sprite as a separate PNG
-- per frame, named "<layer>_<tagframe>.png" in the directory passed as
-- --script-param outdir=<path>.
--
-- Usage:
--   aseprite -b file.aseprite \
--       --script-param outdir=/tmp/frames \
--       --script scripts/export_leaf_frames.lua

local outdir = app.params["outdir"]
if not outdir or outdir == "" then
  error("missing required --script-param outdir=<path>")
end

local sprite = app.activeSprite
if not sprite then
  error("no active sprite")
end

local function collectAll(parent, all)
  for _, layer in ipairs(parent.layers) do
    table.insert(all, layer)
    if layer.isGroup then collectAll(layer, all) end
  end
end

local all = {}
collectAll(sprite, all)

-- A leaf is "wanted" only if it was originally visible AND every ancestor
-- group up to the root was originally visible. Hidden layers are typically
-- reference/backdrop art the artist doesn't want shipped.
local function originallyVisible(layer)
  while layer do
    if not layer.isVisible then return false end
    layer = layer.parent
    if layer == sprite then return true end
  end
  return true
end

local saved = {}
for _, layer in ipairs(all) do
  saved[layer] = layer.isVisible
end

local wantedLeaves = {}
for _, layer in ipairs(all) do
  if not layer.isGroup and originallyVisible(layer) then
    table.insert(wantedLeaves, layer)
  end
end

-- Hide every leaf; keep groups visible so we can re-show leaves one at a time.
for _, layer in ipairs(all) do
  if layer.isGroup then
    layer.isVisible = true
  else
    layer.isVisible = false
  end
end

local exported = 0
for _, layer in ipairs(wantedLeaves) do
  layer.isVisible = true
  -- Skip frames the artist never drew on this layer (no cel). Renumber
  -- surviving frames from 0 so output indices match animations.json's
  -- compact per-animation indexing.
  local outIdx = 0
  for f = 1, #sprite.frames do
    if layer:cel(f) ~= nil then
      local img = Image(sprite.spec)
      img:drawSprite(sprite, f)
      img:saveAs(string.format("%s/%s_%d.png", outdir, layer.name, outIdx))
      outIdx = outIdx + 1
      exported = exported + 1
    end
  end
  layer.isVisible = false
end

for layer, vis in pairs(saved) do
  layer.isVisible = vis
end

print(string.format("exported %d frame(s) from %d leaf layer(s)", exported, #wantedLeaves))
