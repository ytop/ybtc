var CASINO = artifacts.require("./CASINO.sol");

contract('CASINO', function(accounts) {
  it("should return total player", function() {
    var meta;
    var CASINOPlayer;

    return CASINO.deployed().then(function(instance) {
      meta = instance;
      return meta.getTotalPlayer.call();
    }).then(function(totalPlayer) {
      CASINOPlayer = totalPlayer.toNumber();
      return CASINOPlayer;
    }).then(function(CASINOPlayer) {
      assert.equal(1, CASINOPlayer, "play number is not 1");
    });
  });

  it("should return total supply", function() {
    var meta;

    return CASINO.deployed().then(function(instance) {
      meta = instance;
      return meta.balanceOf.call();
    }).then(function(b)  {
      assert.equal(21000000, b, "Not match total supply");
    });
  });

  it("should refill account 1", function() {
    var meta;

    return CASINO.deployed().then(function(instance) {
      meta = instance;
      return meta.refill.call();
    }).then(function(b)  {
      assert.equal(21000020, b, "Not match total supply");
    });
  });

  it("should refill and balance account 1", function() {
    var meta;

    return CASINO.deployed().then(function(instance) {
      meta = instance;
      return meta.refill.call();
    }).then(function(b)  {
      b2 = meta.balanceOf();
      assert.equal(b, 21000020, b, "Not match total supply");
      return b2;
    }).then(function(b2)  {
      assert.equal(b2 ,21000000, "Not match total supply after balance");
    });
  });

  it("should isWinnerMe", function() {
    var meta;
    var testPhase = 0;

    return CASINO.deployed().then(function(instance) {
      meta = instance;
      return meta.isWinnerMe.call(testPhase);
    }).then(function(b)  {
      assert.equal(b, 1,  "Not match phase");
    });
  });

  it("should setNextWinner", function() {
    var meta;
    var testPhase = 0;
    var winnerList = 0x10002

    return CASINO.deployed().then(function(instance) {
      meta = instance;
      return meta.setNextWinners.call(testPhase, winnerList);
    }).then(function(b)  {
      assert.equal(b, true,  "Not match phase");
    });
  });


});
