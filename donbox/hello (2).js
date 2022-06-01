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

function get_paths(tokenA, tokenB) {
  let pair = [tokenA, tokenB].sort().join("-");

  let market = pair_market_map[pair];

  let _paths;

  if (market) {
    _paths = pair;
  } else {
    for (let i = 0; i < paths.length; i++) {
      let path = paths[i];
      let tks = path.split("-");
      if ((tks[0] === tokenA && tks[tks.length - 1] === tokenB)) {
        _paths = path;
        break;
      }
    }
  }
  let mids;

  let tks = _paths.split("-");

  for (let i = 0; i < tks.length - 1; i++) {
    let pair = tks[i] + "-" + tks[i + 1]
    if (!mids) {
      mids = pair_market_map[pair].mid;
    } else {
      mids = mids + "-" + pair_market_map[pair].mid;
    }
  }

  console.log(_paths, mids);

  return mids + "";
}

// 计算兑换路径
let mids3 = get_paths("eosiotoken:WAX2", "eosio.token:WAX");

get_amounts_out(mids3, "eosiotoken:WAX2", 100000000);




function get_amounts_out(mids, token_in, amount_in) {
  let mid_arr = mids.split("-");

  for (let i = 0; i < 10; i++) { if (i > mid_arr.length - 1) { break; }

    let mid = mid_arr[i];
    let swap_result = swap(mid, token_in, amount_in);

    amount_in = swap_result.amount_out;
    token_in = swap_result.token_out;
    quantity_out = swap_result.quantity_out;
  }
  console.log(amount_in, token_in, quantity_out);
}

function swap(mid, token_in, amount_in) {
  let market = mid_market_map[mid];

  let tokenA = market.contract0 + ":" + market.sym0;
  let tokenB = market.contract1 + ":" + market.sym1;
  let precA = market.reserve0.split(" ")[0].split(".")[1].length;
  let precB = market.reserve1.split(" ")[0].split(".")[1].length;


  amount_in -= amount_in * 0.001; // 协议费扣除
  let amount_out;
  let token_out;
  let quantity_out;

  if (token_in === tokenA) {
    let reserve_in = parseFloat(market.reserve0) * (10 ** precA);
    let reserve_out = parseFloat(market.reserve1) * (10 ** precB);
    amount_out = get_amount_out(amount_in, reserve_in, reserve_out);

    token_out = tokenB;
    quantity_out = (amount_out / (10 ** precB)).toFixed(precB) + " " + market.reserve1.split(" ")[1];
	  console.log("quantity_out:", quantity_out);
  }
  if (token_in === tokenB) {
    let reserve_in = parseFloat(market.reserve1) * (10 ** precB);
    let reserve_out = parseFloat(market.reserve0) * (10 ** precA);
    amount_out = get_amount_out(amount_in, reserve_in, reserve_out);

    token_out = tokenA;
    quantity_out = (amount_out / (10 ** precA)).toFixed(precA) + " " + market.reserve0.split(" ")[1];
	  console.log("quantity_out:", quantity_out);
  }

  return {
    token_out,
    amount_out,
    quantity_out
  }
}

function get_amount_out(amount_in, reserve_in, reserve_out) {
  check(amount_in > 0, "invalid input amount");
  check(reserve_in > 0 && reserve_out > 0, "insufficient liquidity");

  let amount_in_with_fee = amount_in * (10000 - 20);
  let numerator = amount_in_with_fee * reserve_out;
  let denominator = reserve_in * 10000 + amount_in_with_fee;
  let amount_out = numerator / denominator;

  check(amount_out > 0, "invalid output amount");
  return amount_out;
}

function check(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}
