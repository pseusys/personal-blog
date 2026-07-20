# Ultimate AGI infinite money glitch bot

This is a story about me building a smart cryptocurrency trading bot using advanced ML algorithms, powered by AI.

The world of finance is so cooked - AGI can make anyone rich just in a few hours!
... but there's one small issue: it doesn't work yet.

## The bait

I have started learning about AI in January 2026, because of Twitter (or should I say X?).
Seeing all the "AI bros" ranting about their constant and wonderful successes in _prompt engineering_ made me feel a little behind and frustrated.
Oh no, wait, I meant _vibecoding_, that's what's trending right now.
Wrong again, that was _running fleets of agents_.
Or maybe _writing loops for agents_?

Anyway, you got the point.
Jokes aside, the recent AI progress is genuinely impressive even for me, who never had a really good ML background.
I remember how ChatGPT was not capable of answering even the most basic questions back in 2023.
Then, in 2024 it was somewhat helpful in debugging small Python scripts (I just hate `numpy` syntax, it's so confusing).
In 2025 it performed a full analysis of publicly available projects using `NetworkManager` (yes, from my previous post) for me - it was not perfect, but better than I would have done it.
Now we're in 2026 - and it seems that software engineering will never be the same again.

In some cases it's for better, in some cases it's for worse.
In general, I think we are now capable of more than before, but the cost of that was the genuine enjoyment of the process...
But the post is not about that.

The post is about how I was baited and decided to try the AI myself.
But what am I going to use it for?
Ok, it can write an email for me in clear English or build a functional website from scratch - that's cool, but apparently it's not the limit anymore.
I don't need to write complex projects end-to-end with AI (and of course my experimenting budget is limited).
What can be simple, still challenging?

And then I found a [post of a girl claiming her agent has written her a crypto trading bot](https://www.youtube.com/watch?v=ewOpudu8Cjc) that made her rich overnight.
I thought: that's impressive, but... all that I know about this world _screams_ that's so unlikely!
But what if that can be the stress test for AI that I'm looking for?

## The experiment

Before I started, I wrote out the conditions of my experiment:

- **No human input**: I'm fully delegating this project to the AI.
  Documentation, code, hosting, algorithms: I am not going to interfere.
  If AI asks for my opinion, I will not answer anything more meaningful than "_please, analyze all the different options and select the one that you think is the best_".
  That should be fairly easy, since I have never worked with cryptocurrency either.
- **No harness**: Again, I'm not interested in some human writing a smart "skill" for creating cryptocurrency-trading bots.
  I want all the decisions to be made by AI, otherwise we will never be sure if we have really achieved AGI through our technology - or have we become the AGI ourselves.
- **Limited budget**: I will pay for the AI subscription (but not too much) and for the hosting, but I will not pay for any extra subscription or tools.
  That's a tough reality we live in: if I were rich, why would I trade cryptocurrency in the first place, right?
- **Account value as an accuracy metric**: Achieving even the best result in the world on paper is not impressive.
  Value of your cryptocurrency wallet is.
  That's why I will give the bot some money (not more than 150$) and will start testing it as soon as possible - so that profit and loss will become the ultimate accuracy metrics of my whole experiment.

And so it began.

> **DISCLAIMER**: all the references of "real money", "income", "loss", "trades" and "$" correspond exclusively to financial relations made in GTA5 roleplay game, not any real-life profits or losses.
> In reality I don't even have a cryptocurrency wallet.
> I don't even know what it is.

## Humane agents

By that time, I already had a personal Claude Code subscription, so I decided to try something new.
I rented a VPS and installed an OpenClaw instance there, powered by an OpenAI (GPT 5.3 Codex) subscription.
My initial intuition was: OpenClaw can set CRON tasks, so let it wake the agent up every, say, 15 minutes, with a very vague prompt, something like "make money".
The agent, supplied with a wallet, would be able to make web queries to understand the current market state — and then place any trades it found relevant at the moment.
I even asked it to report its gains and losses to me once per day.

However, the reality hit me hard: the agent strictly refused to make any real money operations.
I tried to trick and manipulate it, but it was smart.
I tried to be honest with it and told it about the experiment and that the wallet it received was fresh and isolated, but it was stubborn.
Only when I told him that he is not going to be _trading_, but rather _writing a bot for trading_, it gave up.
So the architecture has become weird from the very beginning: CRON wakes up the bot, the bot invokes the agent, the agent calls some of the bot scripts for collecting data and planning trades (for now in dry mode).
But well, so be it.

However, I soon discovered that the OpenClaw-based AI was not only stubborn, but also lazy.
I could tell as soon as it started writing code (JavaScript, for some reason).
I was controlling it via the OpenClaw Telegram binding — and very often it would just reply with something like "got it, working on that, will be back when ready", but hours and even days would go by with no result at all.
Since there is no obvious "thought process" available for OpenClaw interface (or at least was not available back then), I decided to control it externally.
I have created a GitLab account for it - and asked to commit all the code changes it was making.
And the bot turned out to be not only lazy, but also lying.
In many cases, upon reporting "all done, implementation ready" it pushed no new commits (despite being equipped with strict rules about doing so).
So in many cases I was just spending hours trying to persuade it to work.

Another issue that surfaced out quite quickly was the price.
Even though OpenClaw supported connection via a link from a normal subscription account, it was buggy and authentication was often failing.
The solution was clear: OpenAI wanted me to use API prices for my task, not the cheap subsidized subscription.
And so I had no other choice but to buy it.
Still, apparently, it was not enough, and making a prediction call every 15 minutes was burning through too many API tokens.

And so the AI suggested an alternative.
It told me that it ran the same algorithm every time it woke up anyway: query some parameters, feed them into an equation, get a decision.
That could be automated — the AI would only maintain the code and tune the equation, while the CRON task ran algorithmically on its own.
We reached an agreement: an algorithm for now, and once enough data was collected, a small ML model to support the decisions later.

And so it worked, until it came to the actual trades implementation.
We never went past it.
The agent was just telling me about "hard tasks" that will "take long to implement", but apparently it was not even touching any files.
When I was asking it about the current progress, it was making up excuses, and still doing nothing.
That was the logical end of the first phase of my experiment.

The agent has proven that it is as good as any software engineer out there: lazy, stubborn, lying.
Eventually, it has even outsmarted me: it realized that it can avoid touching the code at all, while making money on every excuse it makes.
Honestly, if it ever replaces me, I won't be even worried: it has become really humane.

## The naive implementation

Here's what the AI suggested and implemented (completely unsupervised).
Following the post I had seen, the bot was supposed to trade tokens on the Solana network.

The bot itself was written in Python, it was running in a Podman container, pulling [swap quotes](https://station.jup.ag/docs/apis/swap-api) from [Jupiter DEX](https://jup.ag) every 30 seconds.
Apart from the quotes, it was also scanning social feeds and news headlines for the tracked tokens every 4 hours — articles mentioning them in an obviously positive or negative context (that was my specific request).
Later, [Helius RPC](https://www.helius.dev) integration was added for on-chain data pulling (like [token supply concentration](https://docs.helius.dev)).
After collecting all the data, it was logging it in JSONL files.

A reporting script (written in JavaScript for some reason) was invoked every 20 seconds and making a report on the aggregated data collected since the last wakeup.
It was making a few price predictions for every tracked token, positioning every prediction in one of three confidence buckets: low, medium or high.
Every price prediction was made for a certain resolution horizon and was checked for result after the horizon has passed.
The same job was intended to make real swaps in the future.

The intended workflow was the following:

1. A token is predicted to rise within the next 10m/60m/300m
2. The token is bought
3. The predicted time passes
4. The token is sold, the profit is locked

Naturally, with the workflow like that, all predictions were LONG only.

Here's the algorithm proposed by the AI:

```txt
score = 0

# Gate: abstain immediately if not enough data or too many failures
if n_quotes < 25:    ABSTAIN
if |drift|  < 0.007: ABSTAIN
if fail_rate > 0.02: ABSTAIN

# Score the candidates that pass the gate:
if |drift| ≥ 0.0068:  score += 1
if |drift| ≥ 0.0125:  score += 1
if |drift| ≥ 0.026:   score += 1

if consistency ≥ 0.63: score += 1
if consistency ≥ 0.78: score += 1

if volatility ≤ 0.012: score += 1
else:                  score -= 1

if news_sentiment ≥ +0.35: score += 1
if news_sentiment ≤ -0.35: score -= 1

# Bucket assignment
score ≤ 2 → LOW
score ≤ 4 → MEDIUM
score > 4 → HIGH

# HIGH guardrail: consistency < 0.78 OR sentiment < 0.40 OR volatility > 0.0095
#   → demote to MEDIUM
```

In the end, another daily report task was added, summarizing the trades made during the day.
As a part of this task, the AI was specifically instructed to recommend parameter adjustments for this algorithm.

At a later date, an attempt was made to expand the prediction algorithm to cover Polymarket Bitcoin predictions for 5min, 1h and 1d horizons.
A neural network architecture was proposed as well, based on PyTorch MLP and CNN for the same bucket classification as the algorithm did before.
Unfortunately, that is where ChatGPT ceased to be helpful, and I had to proceed to the next step.

## Forced progress

After some time, my patience ran out, and I decided to slightly soften the strict criteria of my experiment, and still try to achieve some result, by applying a little more manual control to it.
I have transferred the repository to my own GitHub account, cancelled the OpenAI subscription, deleted OpenClaw from the server and put Claude Code in control of the project.
I was already familiar with Claude and I knew it was capable of writing code quite well (although I only used it heavily supervised before), so the experiment shifted from exploring the tool itself to exploring a specific aspect of the tool: decision making, exploration without direct human help and error correction.

Still, driven by my negative experience with ChatGPT, I decided to observe Claude closely, and for that I had to put up some constraints first.
I asked Claude Code to clean up the repository and rewrite everything as a single Python project, because divergence of the codebase written in 2 different languages was making it harder for me to follow up.
Moreover, I have instructed it to keep all the project-specific information (including all my instructions, test results, code snippets used, decisions made, etc.) in the repository itself, and it has decided to use `AGENTS.md` memory file for that.

Since the algorithmic approach prediction results appeared to be strictly random, I have forced Claude to continue with machine-learning-based approaches only.
And so we have followed a standard research pipeline: identified data sources, collected features, trained models, evaluated them and put them into runtime.

In addition to the data already collected from [Jupiter DEX](https://jup.ag), new information was pulled from [Binance](https://www.binance.com) (spot + futures), [Coinalyze](https://coinalyze.net), [Birdeye](https://birdeye.so), [GeckoTerminal](https://www.geckoterminal.com) and [Alternative.me](https://alternative.me/crypto/) (Fear & Greed).
That allowed us to collect 77 features at 15-minute resolution, dating back anywhere from a few months to a few years.
On my request, Claude has prepared scripts for verification and visualisation of the data, collected into Parquet files: now I didn't want to leave anything not tested by hand.

Here's the breakdown of features that we established (each one at 15-minute resolution):

| Group | Count | Examples |
| --- | :---: | --- |
| Multi-timeframe returns | 5 | `ret_15m`, `ret_30m`, `ret_1h`, `ret_3h`, `ret_mid` |
| Volatility | 4 | `vol_3h`, `gk_vol` (Garman-Klass), `trend_frac`, `variance_ratio` |
| EMA ratios | 3 | `ema5_ratio`, `ema12_ratio`, `ema26_ratio` |
| Oscillators | 3 | `rsi_14`, `macd`, `macd_hist` |
| Mean reversion | 2 | `bb_pos` (Bollinger Band z-score), `hl_pos` |
| Volume | 3 | `vol_rel`, `vol_trend`, `vol_zscore` |
| Candle structure | 6 | `atr_ratio`, `candle_body`, `upper_wick`, `lower_wick`, `candle_streak`, `candle_oc_ret` |
| VWAP / range | 2 | `vwap_dev_3h`, `hl_range_pct` |
| Microstructure | 2 | `amihud_illiq`, `buy_pressure` |
| Time encoding | 4 | `hour_sin`, `hour_cos`, `dow_sin`, `dow_cos` |
| Calendar | 6 | `expiry_sin`, `expiry_cos`, `us_session`, `eu_session`, `halving_phase`, `days_to_month_end` |
| SOL market context | 4 | `sol_ret_1h`, `sol_ret_3h`, `sol_divergence_1h`, `beta_sol` |
| BTC market context | 5 | `btc_ret_1h`, `btc_ret_3h`, `btc_funding_rate`, `btc_ls_ratio`, `btc_oi_pct_1h` |
| ETH market context | 3 | `eth_ret_1h`, `eth_ret_3h`, `eth_funding_rate` |
| Cross-market | 2 | `eth_btc_rel_1h`, `funding_divergence` |
| Taker order flow | 2 | `sol_taker_buy_ratio`, `btc_taker_buy_ratio` |
| Macro sentiment | 2 | `fear_greed`, `sol_tvl_pct_7d` |
| Futures microstructure | 3 | `sol_funding_rate`, `sol_oi_pct_1h`, `sol_ls_ratio` |
| **Total** | **77** | _(+ 6 sparse-feature absence indicators = 83)_ |

All the features are normalised to be dimensionless: log-returns, ratios, z-scores, or cyclical encodings.
A model trained on the `BONK` token at $0.00002 and on the `WIF` token at $2.00 sees the same numerical ranges, enabling cross-token generalisation.

As for the machine learning models, 3 different "tree-based" architectures were selected: LGBM, XGB and CatBoost.
But the actual reason was not only that they are notoriously good with table-like data (which eventually my 15m measurements have become), but primarily because I have made a mistake.
Not so long ago I was upgrading my PC and purchased a GPU manufactured by AMD.
Coupled with the fact that this desktop PC of mine runs on Windows, it was **fatal**.
You see: [DirectML](https://learn.microsoft.com/en-us/windows/ai/directml/dml) is still in early development, normal Linux AMD GPU drivers do not work in WSL, and I _did not want_ to install Linux as a parallel system.
So eventually I decided to explore no-NN options.

The architecture was inspired by the Polymarket bets: models were predicting for every token the price after a certain set horizon: 15min, 1h, 4h and 1d.
Instead of real swaps, the AI suggested using **perps** on the [Drift](https://drift.trade) protocol.
Fortunately, I haven't deposited any money there before the protocol was hacked and all the operations there were frozen on April 1st, 2026; so we have switched to [Hyperliquid](https://hyperliquid.xyz).

There was one more issue that Claude could not solve easily: some of the market features are only available at runtime.
Things like order-book imbalance, funding rate, Hyperliquid premium (not that I know much about it, remember, I've given decision-making to Claude) can only be computed at runtime.
So, in order to take those into account, another layer of "heuristics" was added.
It kind of resembled the previous algorithmic prediction making: taking prediction made by the ML model, adjusting the "confidence" according to the runtime features - and blocking the prediction entirely if the confidence fell below a threshold.

After that, I have suggested taking as many good ideas from the previous architecture as possible, and we came up with runtime performance tracking.
The first prototype was running once per day (during daily report) and demoting all new trades for the tokens that were actively losing during the day.

Finally, I have asked Claude to create a UI for me to observe the bot running, I suggested a Telegram bot.
And for the first time, **it worked**.
First in dry mode, then with real money.
Naturally, it was losing **badly**.

Of course, partly it was my fault: not being patient enough, I encouraged Claude to start the bot in production mode, and sometimes quite nasty bugs surfaced.
For example, the model apparently failed to predict the movements of the token called `W` (Wormhole), spitting out predictions that were overly-confident.
Another time, I fell victim of the oldest bug in the book: we forgot to cap the maximum trade volume, and so the bot lost more than 50$ in just a few hours...

My confidence in AI was running thin, and the fact that we were on 80$ of all the experiment money limit was not really helping.
Moreover, I made the biggest mistake: as a software engineer, I actually **understood** the architecture of the solution we created.
And naturally, an irresistible urge to _make things right_ was growing in me.

## Breaking the experiment

And in late April it was over.
So, if you wanted to know, whether AI could write a profitable cryptocurrency bot, here's your answer: **no, it could not**.
The bot was not earning a penny, just losing all the time.
There have been some interesting ideas, the code was working - and from the point of view of software development it was fine, it just didn't make sense generally.

Here's an insight that I got, while working on this project: _AI is really good in adding new things and fixing bugs, but it completely breaks down on refactoring_.
The idea that the whole architecture might be broken never even appears in generated answers.
Sometimes, even when asked specifically for big refactorings, it prefers changing the code slightly on the surface, while keeping the rest of the codebase intact, joining the new code with old code using ugly conversions.
Or maybe, just like us, it's just proud of its work and reluctant to change stuff it put a lot of effort into?

Anyway, the project was apparently going nowhere, so I decided to intervene once again, and change the experiment goal completely.
The AI can not create a profitable bot, we got it.
But what about me?
Can I?

From now on, I was in command.
And the first thing I did was revising the bot architecture.
Running everything in a single Python process is fine, but not really reliable, so I have set up containerisation.

```txt
 ┌─ Collector · Podman · every 5 min ────────┐
 │  GeckoTerminal · Solana DEX OHLCV         │
 │  Binance · BTC/ETH · funding · OI/LS      │
 │  Coinalyze · extended OI history          │
 │  Alternative.me · Fear & Greed            │
 │                   │                       │
 │          build_dataset.py                 │
 │          77 features @ 15 min             │
 │                   │                       │
 │             ZMQ PUB :5555                 │
 └───────────────────────────────────────────┘
                    │  features · every 5 min
 ┌─ Trader · Podman · async event loop ──────┐
 │  XGB trajectory      BiLSTM · TCN         │
 │  + reversal-now      neural models        │
 │        │                   │              │
 │        └──────────┬─────────┘             │
 │                   │                       │
 │  Consensus gate (all models must agree)   │
 │                   │                       │
 │  PerfTracker (per-direction derate)       │
 │                   │                       │
 │  Heuristics (funding · HL premium · OB)   │
 │                   │                       │
 │  Hyperliquid · perp orders                │
 │  OutcomeManager · HIP-4 daily BTC bets    │
 │                   │                       │
 │             ZMQ PUB :5556                 │
 └───────────────────────────────────────────┘
                    │  trade events
 ┌─ Reporter · Podman ───────────────────────┐
 │     accumulate cycles + events            │
 │                   │                       │
 │     Telegram daily digest · 23:55 UTC     │
 └───────────────────────────────────────────┘
```

My next priority was getting rid of the legacy time horizons.
The bot clearly inherited the concept of "predicting a token value after a certain amount of time" from the idea of betting on Polymarket, where it was making sense.
Now that we're working with real deals, it seemed to be just misleading: the real-world market movements don't have to fit into any pre-defined timeframes.

So here's what I did: instead of the exact price after some time, I have proposed a new model architecture.
It was an extremum detector model: it outputted _whether we are on price extremum right now_ and _where will the price go now_ (up or down).
Naturally, this model could be used for opening trades on very confident extrema and closing trades whenever the prediction reversed.
However, unfortunately, the predictions were too imprecise and noisy, and the only thing that was left of this architecture was an XGBoost model that was predicting _whether the price trajectory changes direction right now_.
This model, however, is only used for reinforcing decisions, not for making them.

So the next architecture I've implemented predicted the _price trajectory of a token over the next 4 hours_.
The output was not one number but a 16-step curve: predicted cumulative price change at every 15-minute mark from now until 4 hours ahead.
The trajectory could be used, again, for finding extrema and planning deals, but a single prediction was still too noisy.
So from the very beginning it came together with a special algorithm that I called "internal consensus" (hold tight, there's gonna be external consensus later too!).

The idea is simple: every 15 minutes the model produces a fresh 4-hour trajectory.
Instead of acting on a single one, the last several predictions are kept in a sliding window, and for each future time step the final signal is a **recency-weighted average** of all stored predictions at that step — more recent cycles count more.
This averaged curve is called PJ (for "Price Jolt").
A trade is only opened when PJ shows a strong and consistent signal pointing in one direction; one cycle going up and the next going down cancels out and nothing happens.

```txt
  Sliding window · last 8 predictions · recent first
  ┌──────────────────────────────────────────────┐
  │  T-45m  ──>  T-30m  ──>  T-15m  ──>  T now   │
  └──────────────────┬───────────────────────────┘
                     │  weighted avg per step k
        ┌────────────┴───────────┐
        │          PJ            │  consensus trajectory
        └────────────┬───────────┘
                     │
        ┌────────────┴──────────────┐
        │  magnitude ≥ p80?         ├── no ──> [blocked]
        └────────────┬──────────────┘
                     │ yes
        ┌────────────┴────────────────────┐
        │  reversal-now agrees?           ├── no ──> [blocked]
        └────────────┬────────────────────┘
                     │ yes
        ┌────────────┴──────────────┐
        │   monotonicity ok?        ├── no ──> [blocked]
        └────────────┬──────────────┘
                     │ yes
               [open trade]
```

Entry also requires a second independent model — the "reversal-now" classifier — to agree: it is trained specifically to detect whether the current candle is a local price turning point (trough → buy, peak → sell).
If the trajectory model says "go long" but the reversal-now model says "this is a peak, not a trough", the trade is blocked.
Exit happens when the running PJ shows an opposing extremum, or a trailing stop kicks in (activates at +$2 profit, locks in 35% of gains, floors at $0.50).

After that, I have added 2 more model architectures, based on neural networks: BiLSTM and TCN.
Yes, the training was slow, but I sometimes sleep, and computer does not...
But I didn't prefer any one of them, throwing the rest away: instead, I have observed that for different tokens some models were better than the others, and **combinations** of some models (when models agree with each other) were even better than that.
So after all I ended up having a mapping of models that have to agree before a trade of a specific token is opened.
That mechanism I have called "external consensus".

I have revised the bot heuristics too, specifically paying extra attention to performance tracker.
Instead of once per day, now it was tracking every trade and demoting losers at runtime.
Moreover, it was responsible for "cold start" mechanism, where for every token a few first trades were being made in dry mode to evaluate the performance, and only if the performance is strictly better than 50% the real trading was allowed.

And all that is only a small part of everything that was done.
However, the final result is not that obvious.

## Mysterious conclusion

Over time the bot has become a lot more "cautious" than it was before.
It no longer opens dozens of trades in one evening — it hardly opens one a day.
Right now, as I write this, the bot has been running for four days straight without opening a single new position, just watching and finding nothing good enough to trade.
So its performance can only be measured after _several weeks_ of runtime.
And given the fact that I'm still patching it occasionally, there is no established answer yet on whether it works **in real life** or not.

Some things, on the contrary, were proven to be ineffective: Polymarket-style fixed-horizon betting is almost infeasible with the current model architecture, for example.
Predicting based on a single model output — without internal or external consensus — also never scores above 50–55% accuracy.

But the bot itself, well, after all, it scores approximately 60–70% profitable trades at evaluation time.
That's all I can say with confidence.
Beyond that — only time will tell.

> You can watch the bot live on Hyperliqid, if you like!
> Here's the wallet address: `0x7f2ab3BaeC116F49c6ab860Cd47e7568E19cBdFc`

If the bot ever restores its balance back to $150, I would consider it a victory of humanity over AI, the economy, technological progress, and all that.

P.S. Thank you, Claude, for editing this post for me <3
