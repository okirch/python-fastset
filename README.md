# python fastset

This is an implementation of set-like classes built over a defined "domain" of
potential class members. Due to these restrictions, it can perform set operations
much faster than regular python sets.

## Implementation

With set members being limited to a specific domain of objects, sets can be
represented as bit vectors (a zero bit indicates absence of an object, a one
bit indicates presence of a member). With this encoding, set operations become
operations on bit vectors.

## Using fastset domains and set

```import fastset

ColorDomain = domain("colors")

class Color(ColorDomain.member):
	def __init__(self, ...):
		super().__init__()

ColorSet = ColorDomain.set

RED = Color("RED")
BLUE = Color("BLUE")
FUNKY = Color("LIGHT_SLATE_WITH_A_TINGE_OF_GREEN")


setA = ColorSet((RED, BLUE, FUNKY))
if FUNKY in setA:
	print("A is funky")
setB = setA.difference(ColorSet((RED, BLUE))

´´´
