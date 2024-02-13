# Tracer
commit test

# To-do
- ~~Scene configuration with Json files~~ Done
- Some kind of configuration that sets renderer parameters
- More model format supports
- Add mechanics for determining whether a ray is inside a medium, and which medium it is in
- Add mesh configuration that indicates if the vertices are grouped clockwise or counter-clockwise (this is useful for generating normals that always face the correct side)
- Use normals generated from intersection detection to determine if a ray is leaving the boundary of a medium
- Resolve issues caused by bordering a medium with another object/medium