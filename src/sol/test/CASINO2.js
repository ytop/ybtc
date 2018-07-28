const CASINO = artifacts.require("./CASINO");

contract('2nd CASINO test', async (accounts) => {

  it("should send coin correctly", async () => {

    // Get initial balances of first and second account.
    let account_one = accounts[0];
    let account_two = accounts[1];

    let instance = await CASINO.deployed();
    let meta = instance;

    let balance = await meta.balanceOf.call({from: account_one});
    let account_one_starting_balance = balance.toNumber();

    balance = await meta.refill.call({from: account_one});
    let account_one_ending_balance = balance.toNumber();

    balance = await meta.refill.call({from: account_two});
    let account_two_starting_balance = balance.toNumber();

    balance = await meta.balanceOf.call({from: account_one});
    let account_one_third_balance = balance.toNumber();

    //balance = await meta.balanceOf.call({from: account_two});
    //let account_two_ending_balance = balance.toNumber();

    let players = await meta.getTotalPlayer.call();
    let player_number = players.toNumber();


    assert.equal(account_one_starting_balance, 21000000, "balance one start ");
    assert.equal(account_one_ending_balance, 21000020, "balance one end ");
    assert.equal(account_two_starting_balance, 20, "balance two start ");
    //assert.equal(account_one_third_balance, 21000020, "balance one third ");
    //assert.equal(account_two_ending_balance, 20, "balance two end");
    //assert.equal(player_number, 2, "players ");
  });

})
