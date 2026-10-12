import itertools

# 0-12 ranks (2 to A), 0-3 suits (S, H, D, C)
# Card = rank * 4 + suit

def get_canonical(cards):
    # cards is a list of 3 card ints
    cards = sorted(cards, reverse=True)
    
    # We want to map suits in order of appearance
    # Wait, if two cards have the SAME rank, their order of appearance depends on their original suit!
    # For example, Ah As 2d and As Ah 2d are the same hand.
    # To make the canonical mapping unique, we should try ALL permutations of cards with the same rank?
    # No, it's easier to just try all 24 permutations of suits, apply the permutation to the 3 cards,
    # sort the resulting 3 cards descending, and pick the lexicographically largest (or smallest) 3-card tuple!
    
    best_tuple = None
    
    import math
    suit_perms = list(itertools.permutations([0, 1, 2, 3]))
    
    for perm in suit_perms:
        mapped_cards = []
        for c in cards:
            rank = c // 4
            suit = c % 4
            mapped_c = rank * 4 + perm[suit]
            mapped_cards.append(mapped_c)
        mapped_cards.sort(reverse=True)
        t = tuple(mapped_cards)
        if best_tuple is None or t > best_tuple:
            best_tuple = t
            
    return best_tuple

canonical_counts = {}
all_cards = list(range(52))

for combo in itertools.combinations(all_cards, 3):
    canon = get_canonical(combo)
    if canon not in canonical_counts:
        canonical_counts[canon] = 0
    canonical_counts[canon] += 1
    
print(f"Total canonical flops: {len(canonical_counts)}")
