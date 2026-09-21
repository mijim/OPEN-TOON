"""Contract graph regressions; no application build or network required."""
import copy
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('roadmap', ROOT / 'scripts/roadmap.py')
roadmap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(roadmap)


class RoadmapContractTests(unittest.TestCase):
    def setUp(self):
        self.records = {name: copy.deepcopy(roadmap.read(name)) for name in
                        ('roadmap.json', 'libraries.json', 'node-assignments.json')}
        self.plan = self.records['roadmap.json']

    def errors(self):
        with patch.object(roadmap, 'read', side_effect=self.records.__getitem__):
            return roadmap.validate()

    def rejects(self, fragment):
        self.assertTrue(any(fragment in error for error in self.errors()), fragment)

    def test_current_plan_is_consistent(self):
        self.assertEqual(self.errors(), [])

    def test_cycle_cannot_masquerade_as_a_prerequisite(self):
        self.plan['delivery_slices'][0]['requires'].append(
            {'slice': 'HM-15', 'contract': 'Invalid feedback edge'})
        self.rejects('Delivery dependency cycle')

    def test_unknown_contract_provider_is_rejected(self):
        self.plan['delivery_slices'][1]['requires'][0]['slice'] = 'HM-99'
        self.rejects('Unknown delivery prerequisite')

    def test_missing_contract_reason_is_rejected(self):
        self.plan['delivery_slices'][1]['requires'][0]['contract'] = ''
        self.rejects('Missing/duplicate prerequisite contract')

    def test_required_artwork_branch_cannot_be_orphaned(self):
        self.plan['delivery_slices'][3]['requires'] = [
            edge for edge in self.plan['delivery_slices'][3]['requires']
            if edge['slice'] != 'HM-02']
        self.rejects('terminal gate does not depend on every required slice')

    def test_a_feature_cannot_be_both_required_and_deferred(self):
        next(a for a in self.plan['feature_assignments']
             if a['feature_id'] == 'RIG-015')['delivery_priority'] = 'long_term'
        self.rejects('Contradictory delivery priority')

    def test_completion_requires_delivered_parents(self):
        terminal = self.plan['delivery_slices'][-1]
        terminal['status'] = 'complete'
        terminal['completion_evidence'] = ['claimed review']
        self.rejects('Unproven delivery completion')

    def test_execution_order_cannot_start_consumers_first(self):
        self.plan['execution_policy']['priority_order'].reverse()
        self.rejects('Delivery priority precedes prerequisite')

    def test_duplicate_primary_ownership_remains_invalid(self):
        self.plan['feature_assignments'].append(copy.deepcopy(self.plan['feature_assignments'][0]))
        self.rejects('exactly one primary phase assignment')


if __name__ == '__main__':
    unittest.main()
