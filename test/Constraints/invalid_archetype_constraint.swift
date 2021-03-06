// RUN: %target-parse-verify-swift

protocol Empty {}

protocol P {
  associatedtype Element
  init()
}

struct A<T> : P {
  typealias Element = T
}

struct A1<T> : P {
  typealias Element = T
}

struct A2<T> : P {
  typealias Element = T
}

func toA<S: Empty, AT:P where AT.Element == S.Generator.Element>(_ s: S) -> AT { // expected-error{{'Generator' is not a member type of 'S'}}
  return AT()
}
