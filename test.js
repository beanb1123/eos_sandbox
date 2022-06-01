import * as unittest from 'unittest';
import { Converter, Pool } from 'trading_arbitrator/primitives';
import { constant_sum_amm } from 'trading_arbitrator/amm';
import { Arbitrator } from 'trading_arbitrator/arbitrator';

var _pj;

function _pj_snippets(container) {
  function in_es6(left, right) {
    if (right instanceof Array || typeof right === "string") {
      return right.indexOf(left) > -1;
    } else {
      if (right instanceof Map || right instanceof Set || right instanceof WeakMap || right instanceof WeakSet) {
        return right.has(left);
      } else {
        return left in right;
      }
    }
  }

  container["in_es6"] = in_es6;
  return container;
}

_pj = {};

_pj_snippets(_pj);

class ArbitratorTests extends unittest.TestCase {
  setUp() {
    this.pairs_simple = [["A", "B"], ["A", "C"], ["B", "C"]];
    this.assets = ["A", "B", "C"];
    this.assets2 = ["A", "B", "C", "D"];
    this.rates = [1.2, 0.8, 1.1];
    this.amounts = [100, 200, 300];
    this.amounts2 = [100, 200, 300, 400];
    this.converter = new Converter("testC", {
      "conversion_formula": constant_sum_amm
    });
    this.p1 = new Pool("test1", {
      "assets": this.assets,
      "amounts": this.amounts,
      "converter": this.converter
    });
    this.p2 = new Pool("test2", {
      "assets": this.assets2,
      "amounts": this.amounts2,
      "converter": this.converter
    });
  }

  test_admits_simple_input() {
    var arb, expected, loop, loops;
    arb = new Arbitrator({
      "pairs": this.pairs_simple,
      "rates": this.rates,
      "initial_assets": ["A"]
    });
    loops = arb.get_loops({
      "sizes": [3]
    });
    this.assertEqual(loops.length, 2);
    loop = loops[0];
    this.assertEqual(loop.size, 3);
    expected = [["A", "B"], ["A", "C"], ["B", "C"]];

    for (var pair, _pj_c = 0, _pj_a = loop.pairs, _pj_b = _pj_a.length; _pj_c < _pj_b; _pj_c += 1) {
      pair = _pj_a[_pj_c];
      this.assertTrue(_pj.in_es6([pair.asset0, pair.asset1], expected));
    }
  }

  test_admits_complex_pools() {
    var arb, loops;
    arb = new Arbitrator({
      "pools": [this.p1, this.p2],
      "initial_assets": ["A", "B"]
    });
    loops = arb.get_loops({
      "sizes": [3, 4]
    });

    for (var loop, _pj_c = 0, _pj_a = loops, _pj_b = _pj_a.length; _pj_c < _pj_b; _pj_c += 1) {
      loop = _pj_a[_pj_c];
      this.assertTrue(loop.size > 2 && loop.size < 5);
      this.assertTrue(_pj.in_es6(loop.initial_asset, ["A", "B"]));
    }
  }

}
