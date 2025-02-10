import fastsets
import string
import random
import time

if False:
	debug = print
else:
	def debug(*args, **kwargs): pass

LabelDomain = fastsets.Domain("labels")

class Label(LabelDomain.member):
	def __init__(self, name):
		super().__init__()
		self.name = name
	
	def __str__(self):
		return self.name

class LabelSet(LabelDomain.set):
	def asSet(self):
		return set(self)

class LabelDictionary(dict):
	def add(self, name):
		label = self.get(name)
		if label is None:
			label = Label(name)
			self[label.name] = label
		return label

class FastsetTester:
	def __init__(self, keys):
		self.keys = keys
		self.setsize = len(self.keys)
		self.labelDict = LabelDictionary()

		for s in keys:
			self.labelDict.add(s)

		self.allLabels = list(self.labelDict.values())

	def test(self, numIterations):
		print("******************************************************************")
		print(f"Performing {numIterations} tests of random combinations of sets")
		for i in range(numIterations):
			t.testRandomPair()

		# this should fail safely
		# x = LabelSet.union(set(), LabelSet((1, 2)))

		print(f"Done, tests checked out OK")
		print()

	def performTimingTests(self):
		print("******************************************************************")
		print(f"Perform timing tests for set operations (values in seconds, smaller is better)")
		print(f" {'name':20} {'set':>12} {'fastset':>12}")
		self.timeContains()
		self.timeUnarySetOperation('len', len)
		self.timeUnarySetOperation('bool', bool)
		self.timeSetOperations(('union', 'intersection', 'difference', 'symmetric_difference', 'issubset', 'issuperset', 'isdisjoint'))

	def randomSet(self, klass = set):
		nelements = random.randrange(self.setsize)
		return klass(map(self.labelDict.add, random.choices(self.keys, k = nelements)))

		print(f"Done.")
		print()

	def setToLabelSet(labels):
		result = LabelSet()
		for label in labels:
			result.add(label)
		return result

	def testUnaryFunction(self, name, func, a):
		avec = LabelSet(a)

		r1 = func(a)
		r2 = func(avec)

		if r1 != r2:
			raise Exception(name)

		debug(f" {name} OK")

	def testUnarySetOperation(self, name, a):
		avec = LabelSet(a)

		r1 = getattr(set, name)(a)
		r2 = getattr(avec.__class__, name)(avec)

		if r1 != r2:
			raise Exception(name)

		debug(f" {name} OK")

	def renderLabelSet(self, s):
		def tostr(label):
			s = str(label)
			if s == ' ':
				s = '" "';
			return s

		return f"<{' '.join(sorted(map(tostr, s)))}>"

	def failBinarySetOperation(self, name, a, b, r, avec, bvec, rvec):
		print(f"*** Operation {name} failed")
		print(f"  a = {self.renderLabelSet(a)}")
		print(f"  b = {self.renderLabelSet(b)}")
		print(f"  r = {self.renderLabelSet(r)}")
		print(f"actual result:")
		print(f"  a = {self.renderLabelSet(avec)}")
		print(f"  b = {self.renderLabelSet(bvec)}")
		print(f"  r = {self.renderLabelSet(rvec)}")

		print(f"difference:")
		missing = r.difference(set(rvec))
		if missing:
			print(f"  missing = {self.renderLabelSet(missing)}")
		excess = set(rvec).difference(r)
		if excess:
			print(f"  excess = {self.renderLabelSet(excess)}")

		raise Exception(f"{name} bug encountered")

	def testBinarySetOperation(self, name, a, b):
		avec = LabelSet(a)
		bvec = LabelSet(b)

		r = getattr(set, name)(a, b)
		rvec = getattr(avec.__class__, name)(avec, bvec)

		if r != rvec.asSet():
			self.failBinarySetOperation(name, a, b, r, avec, bvec, rvec)

		debug(f" {name} OK")

	def testBinarySetOperationUpdate(self, name, a, b):
		avec = LabelSet(a)
		bvec = LabelSet(b)

		aOrig = a.copy()
		avecOrig = avec.copy()

		getattr(set, name)(a, b)
		getattr(avec.__class__, name)(avec, bvec)

		if a != avec.asSet():
			self.failBinarySetOperation(name, aOrig, b, a, avecOrig, bvec, avec)
			raise Exception(f"{name} bug")

		debug(f" {name} OK")

	def testBinarySetOperationBool(self, name, a, b):
		avec = LabelSet(a)
		bvec = LabelSet(b)

		b1 = getattr(set, name)(a, b)
		b2 = getattr(avec.__class__, name)(avec, bvec)

		if b1 != b2:
			print(f"Operation {name} test failed")
			print(f" a={' '.join(sorted(map(str, a)))}")
			print(f" b={' '.join(sorted(map(str, b)))}")
			print(f" expected result {b1}, got {b2}")
			raise Exception(name)

		debug(f" {name} OK")

	def testContains(self, a):
		avec = LabelSet(a)
		for label in self.allLabels:
			b1 = label in a
			b2 = label in avec
			assert(b1 == b2)

		debug(f" __contains__ OK")

	def testPop(self, a):
		avec = LabelSet(a)
		control = set()
		while avec:
			control.add(avec.pop())

		if a != control:
			raise Exception("buggy method pop()")

		debug(f" pop() OK")

	def testSetOperations(self, a, b):
		debug("Testing pair of sets")
		for name in 'union', 'intersection', 'difference', 'symmetric_difference':
			self.testBinarySetOperation(name, a, b)

		# for name in 'update', 'intersection_update', 'difference_update', 'symmetric_difference_update':
		for name in 'intersection_update',:
			self.testBinarySetOperationUpdate(name, a, b)

		for name in 'issubset', 'issuperset', 'isdisjoint', '__lt__', '__le__', '__gt__', '__ge__', '__eq__', '__ne__':
			self.testBinarySetOperationBool(name, a, b)

		self.testUnaryFunction('len', len, a)
		self.testUnaryFunction('bool', bool, a)
		self.testContains(a)
		self.testPop(a)

	def testRandomPair(self):
		a = self.randomSet()
		b = self.randomSet()
		e = set()

		self.testSetOperations(a, b)
		self.testSetOperations(e, b)
		self.testSetOperations(a, e)

	def timeBinaryOperation(self, name, klass, iterations = 100, loopcount = 10000):
		func = getattr(klass, name)

		elapsed = 0
		for i in range(iterations):
			a = self.randomSet(klass)
			b = self.randomSet(klass)

			t0 = time.time()
			for k in range(loopcount):
				r = func(a, b)
			elapsed += time.time() - t0

		return elapsed

	def timeUnaryOperation(self, name, func, klass, iterations = 100, loopcount = 10000):
		elapsed = 0
		for i in range(iterations):
			a = self.randomSet(klass)

			t0 = time.time()
			for k in range(loopcount):
				r = func(a)
			elapsed += time.time() - t0

		return elapsed

	def timeMemberTestOperation(self, klass, iterations = 100, loopcount = 5000):
		elapsed = 0;
		for i in range(iterations):
			a = self.randomSet(klass)

			t0 = time.time()
			for k in range(loopcount):
				for label in self.allLabels:
					if label in a:
						pass
			elapsed += time.time() - t0

		return elapsed

	def timeSetOperations(self, operationNames):
		for name in operationNames:
			t1 = t.timeBinaryOperation(name, set)
			t2 = t.timeBinaryOperation(name, LabelSet)

			print(f" {name:20} {t1:12.3} {t2:12.3}")

	def timeUnarySetOperation(self, name, func):
		t1 = t.timeUnaryOperation(name, func, set)
		t2 = t.timeUnaryOperation(name, func, LabelSet)

		print(f" {name:20} {t1:12.3} {t2:12.3}")

	def timeContains(self):
		t1 = t.timeMemberTestOperation(set)
		t2 = t.timeMemberTestOperation(LabelSet)

		name = "contains"
		print(f" {name:20} {t1:12.3} {t2:12.3}")


keys = string.ascii_letters
keys = list(map(str, range(1000)))

t = FastsetTester(keys)
t.test(2000)

t.performTimingTests()

exit(0)
