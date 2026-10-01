import argparse
import json
import sys
from dataclasses import asdict

from .agent import MobilityAgent
from .config import load_config
from .data import MobilityFeatures, synthetic_examples, train_test_split
from .evaluation import evaluate
from .training import train_model


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Run the synthetic LTM experiment")
    subparsers = parser.add_subparsers(dest="command", required=True)
    demo = subparsers.add_parser("demo", help="train, evaluate, and run one inference")
    demo.add_argument("--config", default="config/default.json")
    arguments = parser.parse_args(argv)

    try:
        config = load_config(arguments.config)
        examples = synthetic_examples(config.samples, config.seed)
        training, test = train_test_split(
            examples, config.train_fraction, config.seed
        )
        model = train_model(
            training, epochs=config.epochs, learning_rate=config.learning_rate
        )
        metrics = evaluate(model, test, config.decision_threshold)
        recommendation = MobilityAgent(model, config).recommend(
            MobilityFeatures(neighbor_margin_db=8.0, target_load=0.2)
        )
    except (OSError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    print(
        json.dumps(
            {
                "scope": "synthetic experiment only",
                "evaluation": asdict(metrics),
                "example_recommendation": asdict(recommendation),
                "note": "Advisory output only; no radio procedure is executed.",
            },
            indent=2,
        )
    )
    return 0
