var { abc } = require('./data');
var { inum } = require('./itr');

console.log(inum);
let markets = abc

let pair_market_map = {};
let mid_market_map = {};
let tokens = [];
let paths = [];

markets.map(x => {
  let tokenA = x.contract0 + ":" + x.sym0;
  let tokenB = x.contract1 + ":" + x.sym1;

  let pair_a = tokenA + "-" + tokenB;
  let pair_b = tokenB + "-" + tokenA;

  pair_market_map[pair_a] = x;
  pair_market_map[pair_b] = x;

  mid_market_map[x.mid] = x;

  paths.push(pair_a);
  paths.push(pair_b);

  let new_paths = []

  for (let i = inum; i < paths.length; i++) { if (i > 10) { break; }

    let path = paths[i];
    let tks = path.split("-");
    if (tks[0] === tokenA && tks[tks.length - 1] !== tokenB) {
      new_paths.push(tokenB + "-" + path)
    }

    if (tks[tks.length - 1] === tokenA && tks[0] !== tokenB) {
      new_paths.push(path + "-" + tokenB);
    }

    if (tks[0] === tokenB && tks[tks.length - 1] !== tokenA) {
      new_paths.push(tokenA + "-" + path)
    }

    if (tks[tks.length - 1] === tokenB && tks[0] !== tokenA) {
      new_paths.push(path + "-" + tokenA);
    }
  }

  paths = paths.concat(new_paths);

  if (tokens.indexOf(tokenA) === -1) {
    tokens.push(tokenA)
  }
  if (tokens.indexOf(tokenB) === -1) {
    tokens.push(tokenB)
  }
})

paths = paths.sort((a, b) => {
  return a.length - b.length;
})

  get_paths(tokenA, tokenB, type) {
    const newTokenA = type === 'pay' ? tokenA : tokenB
    const newTokenB = type === 'pay' ? tokenB : tokenA
    if (!this.isInit) return;
    let _paths;
    const _pathsArr = [];

    for (let i = 0; i < this.paths.length; i++) {
      // if (_pathsArr.length === 10) {
      //   break
      // }
      let path = this.paths[i];
      let tks = path.split("-");
      if ((tks[0] === newTokenA && tks[tks.length - 1] === newTokenB)) {
        _paths = path;
        _pathsArr.push(_paths)
      }
    }
    // 根据兑换路径, 找出对应的mid路径
    this._pathsArr = _pathsArr; // 查到所有路径 - 合约路径
    const _pathsMids = [];
    _pathsArr.forEach((v) => {
      let mids;
      let tks = v.split("-");

      for (let i = 0; i < tks.length - 1; i++) {
        let pair = tks[i] + "-" + tks[i + 1]
        if (!mids) {
          mids = this.pair_market_map[pair].mid;
        } else {
          mids = mids + "-" + this.pair_market_map[pair].mid;
        }
      }
      _pathsMids.push(mids + '') // 返回所有Mid路径
    })
    // console.log(_pathsMids)
    // return [_pathsMids[0]];
    return _pathsMids;
  }

  //  mids = [], token_in = eosio.token:EOS, amount_in = 10000, type = 'pay' | 'get'
  get_amounts_out(mids, token_in, amount_in, type) {
    if (!this.isInit) return;
    let amounts_out_arr = [];
    let hasMids = 0;
    mids.forEach((m, mIndex) => {
      let mid_arr = m.split("-");
      if (mid_arr.length === 1) {
        hasMids = mid_arr[0]
      }
      let quantity_out;
      let price = 1;
      let swapInPrice = 1;
      let swapOutPrice = 1;
      let new_token_in = token_in, new_amount_in = amount_in;
      // console.log(' --------------------- ')
      for (let i = 0; i < mid_arr.length; i++) {
        let mid = mid_arr[i];
        // console.log('mid', mid)
        let market = this.markets.find(v => v.mid == mid);
        if (!market) {
          return
        }
        let swap_result
        if (!type) {
          swap_result = this.swap(mid, new_token_in, new_amount_in);
        } else {
          swap_result = this.swap(mid, new_token_in, new_amount_in, type);
        }
        new_amount_in = swap_result.amount_out;
        new_token_in = swap_result.token_out;
        quantity_out = swap_result.quantity_out;
        price = swap_result.price * price;
        swapInPrice = swap_result.swapInPrice * swapInPrice;
        swapOutPrice = swap_result.swapOutPrice * swapOutPrice;
      }
      amounts_out_arr.push({
        amount_in: new_amount_in, token_in: new_token_in, quantity_out, price, mid: m, mIndex,
        swapInPrice, swapOutPrice,
      })
    })
    if (!type) {
      amounts_out_arr.sort((a, b) => {
        return parseFloat(b.quantity_out) - parseFloat(a.quantity_out);
      })
    } else {
      amounts_out_arr = amounts_out_arr.filter(v => v.amount_in > 0)
      amounts_out_arr.sort((a, b) => {
        return parseFloat(a.quantity_out) - parseFloat(b.quantity_out);
      })
    }
    if (!amounts_out_arr.length) {
      return {}
    }
    this.bestPath = this._pathsArr[amounts_out_arr[0].mIndex]
    amounts_out_arr[0].bestPath = this.bestPath;
    amounts_out_arr[0].hasMids = hasMids;

    return amounts_out_arr[0]
  }

  swap(mid, token_in, amount_in, type) {
    if (!this.isInit) return;
    let market = this.markets.find(v => v.mid == mid);
    if (!market) {
      return
    }
    let tokenA = market.contract0 + ":" + market.sym0.split(",")[1];
    let tokenB = market.contract1 + ":" + market.sym1.split(",")[1];
    let inNum = amount_in;
    if (!type) {
      amount_in -= amount_in * 0.001; // 协议费扣除
      inNum = amount_in * 0.998;
    }
    let amount_out;
    let token_out;
    let quantity_out;
    let price;
    let swapPrice; // 兑换后的价格
    if (token_in === tokenA) {
      inNum = inNum / (10 ** market.sym0.split(",")[0]);
      let reserve_in = parseFloat(market.reserve0) * (10 ** market.sym0.split(",")[0]);
      let reserve_out = parseFloat(market.reserve1) * (10 ** market.sym1.split(",")[0]);
      if (!(reserve_in > 0 && reserve_out > 0)) {
        return {
          token_out: tokenB,
          amount_out: '0',
          quantity_out: '0',
          price: '0'
        }
      }
      if (!type) {
        amount_out = this.get_amount_out(amount_in, reserve_in, reserve_out);
      } else {
        amount_out = this.get_amount_in(amount_in, reserve_in, reserve_out);
      }
      token_out = tokenB
      // console.log('tokenA' ,amount_out)
      quantity_out = toFixed((amount_out / (10 ** market.sym1.split(",")[0])), market.sym1.split(",")[0]) + " " + market.reserve1.split(" ")[1];
      if (!type) {
        price = parseFloat(market.reserve1) / parseFloat(market.reserve0);
      } else {
        price = parseFloat(market.reserve0) / parseFloat(market.reserve1);
      }
      swapPrice = accDiv(amount_out, 10 ** market.sym1.split(",")[0]); // 计算总输出 - 不截取
      // console.log('1 ----- ', quantity_out, ' ------- ', price)
    }
    if (token_in === tokenB) {
      inNum = inNum / (10 ** market.sym1.split(",")[0]);
      let reserve_in = parseFloat(market.reserve1) * (10 ** market.sym1.split(",")[0]);
      let reserve_out = parseFloat(market.reserve0) * (10 ** market.sym0.split(",")[0]);
      if (!(reserve_in > 0 && reserve_out > 0)) {
        return {
          token_out: tokenA,
          amount_out: '0',
          quantity_out: '0',
          price: '0'
        }
      }
      if (!type) {
        amount_out = this.get_amount_out(amount_in, reserve_in, reserve_out);
      } else {
        amount_out = this.get_amount_in(amount_in, reserve_in, reserve_out);
      }
      token_out = tokenA;
      // console.log('tokenB' ,amount_out)
      quantity_out = toFixed((amount_out / (10 ** market.sym0.split(",")[0])), (market.sym0.split(",")[0])) + " " + market.reserve0.split(" ")[1];
      if (!type) {
        price = parseFloat(market.reserve0) / parseFloat(market.reserve1);
      } else {
        price = parseFloat(market.reserve1) / parseFloat(market.reserve0);
      }
      // console.log(reserve_out, reserve_in)
      swapPrice = accDiv(amount_out, 10 ** market.sym0.split(",")[0]); // 计算总输出 - 不截取
      // console.log('2 ----- ', quantity_out, ' ------- ', price)
    }
    // console.log('swapPrice', swapPrice)
    let swapInPrice, swapOutPrice;
    if (!type) {
      swapInPrice = swapPrice / inNum;
      swapOutPrice = inNum / swapPrice;
      // console.log(amount_out, inNum, swapPrice)
    } else {
      // const noFeesAmt = accMul(amount_out, 0.998);
      // swapPrice = accDiv(noFeesAmt, 10 ** market.sym0.split(",")[0]); // 计算总输出 - 不截取
      swapPrice = swapPrice * 0.997;
      swapInPrice = inNum / swapPrice;
      swapOutPrice = swapPrice / inNum;
    }
    // console.log(swapInPrice, inNum, price)
    return {
      token_out,
      amount_out,
      quantity_out,
      price,
      swapInPrice,
      swapOutPrice
    }
  }

  get_amount_out(amount_in, reserve_in, reserve_out) {
    if (!this.isInit) return 0;
    if (!(amount_in > 0)) {
      return 0
    }
    let amount_in_with_fee = amount_in * (10000 - 20); // 去除手续费后总输入
    let numerator = amount_in_with_fee * reserve_out;
    let denominator = reserve_in * 10000 + amount_in_with_fee;
    let amount_out = numerator / denominator;
    if (!(amount_out > 0)) {
      return 0
    }
    return amount_out;
  }
  // 根据获得计算输入
  get_amount_in(amount_out, reserve_in, reserve_out) {
    if (!this.isInit) return 0;
    if (!(amount_out > 0)) {
      return 0
    }
    // let numerator = reserve_in * amount_out;
    // let denominator = reserve_out - amount_out;
    // let amount_in_with_fee = numerator / denominator;
    // let amount_in = amount_in_with_fee * 10000 / (10000 - 20);

    let amount_in_with_fee = amount_out * 10000; // 去除手续费后总输入
    let numerator = amount_in_with_fee * reserve_out;
    let denominator = reserve_in * 10000 - amount_in_with_fee;
    let amount_in = numerator / denominator;
    amount_in = amount_in / 0.997
    if (!(amount_in > 0)) {
      return 0
    }
    return amount_in;
  }

  check(condition, message) {
    if (!condition) {
      throw new Error(message);
    }
  }
}
