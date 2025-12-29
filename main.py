import argparse
import sys
from scraper.crawler import FreesoundCrawler
from scraper.gatekeeper import Gatekeeper
from scraper.sorter import Sorter
from scraper.analyzer import Analyzer
from scraper.librarian import Librarian

def main():
    parser = argparse.ArgumentParser(description="AI-Powered Sample Scraper & Organizer")
    parser.add_argument("--query", type=str, required=True, help="Search query (e.g., 'kick drum')")
    parser.add_argument("--count", type=int, default=5, help="Number of samples to download")
    
    args = parser.parse_args()
    
    # 1. Initialize Components
    try:
        crawler = FreesoundCrawler()
    except ValueError as e:
        print(f"Error: {e}")
        sys.exit(1)
        
    gatekeeper = Gatekeeper()
    # Sorter loads a heavy model, might take a moment
    sorter = Sorter()
    analyzer = Analyzer()
    librarian = Librarian()
    
    # 2. Search & Download
    results = crawler.search(args.query, limit=args.count * 2) # Fetch more to account for bad quality
    
    downloaded_count = 0
    # Track hashes to avoid dupes in this session (though gatekeeper should persist this ideally)
    session_hashes = set()
    
    for result in results:
        if downloaded_count >= args.count:
            break
            
        print(f"Processing candidate: {result['name']} (ID: {result['id']})")
        
        # Download
        file_path = crawler.download_sound(result)
        if not file_path:
            continue
            
        # 3. Quality Check
        passed, reason = gatekeeper.check_quality(file_path)
        if not passed:
            print(f"  -> Rejected: {reason}")
            # Delete bad file
            try:
                import os
                os.remove(file_path)
            except:
                pass
            continue
            
        # 4. Duplicate Check
        if gatekeeper.is_duplicate(file_path, session_hashes):
            print(f"  -> Rejected: Duplicate detected")
            try:
                import os
                os.remove(file_path)
            except:
                pass
            continue
        
        # Add hash to session
        file_hash = gatekeeper.get_file_hash(file_path)
        if file_hash:
            session_hashes.add(file_hash)
            
        # 5. Analyze & Sort
        print("  -> Analyzing...")
        classification = sorter.classify(file_path)
        print(f"  -> Classified as: {classification}")
        
        audio_props = analyzer.check_audio_properties(file_path)
        print(f"  -> Properties: {audio_props}")
        
        # 6. Organize
        new_path = librarian.organize_file(file_path, classification, audio_props)
        if new_path:
            print(f"  -> Saved to Library!")
            downloaded_count += 1
            
    print(f"\nDone! Downloaded {downloaded_count} new samples.")

if __name__ == "__main__":
    main()
