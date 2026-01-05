"""
Stem Generator CLI

Command-line interface for stem separation and batch processing.
"""

import sys
from pathlib import Path

import click


@click.group()
@click.version_option(version="1.0.0")
def cli():
    """
    AI Stem Generator - Separate audio into stems using Demucs or LALAL.AI.
    
    Examples:
    
        # Separate a single file
        stem-gen separate song.mp3
        
        # Batch process a directory
        stem-gen batch ./music --output ./stems
        
        # Export to Ableton
        stem-gen export-ableton ./stems/MySong
    """
    pass


@cli.command()
@click.argument('input_file', type=click.Path(exists=True))
@click.option('--engine', '-e', 
              type=click.Choice(['auto', 'demucs', 'lalal']), 
              default='auto',
              help='Separation engine to use')
@click.option('--output', '-o', type=click.Path(), default=None,
              help='Output directory (defaults to data/stems/)')
@click.option('--no-fallback', is_flag=True,
              help='Disable quality-based fallback to other engines')
def separate(input_file: str, engine: str, output: str, no_fallback: bool):
    """
    Separate a single audio file into stems.
    
    Outputs 4 stems: vocals, drums, bass, other
    """
    from ..core.stem_pipeline import StemPipeline
    
    click.echo(f"[*] Processing: {input_file}")
    click.echo(f"   Engine: {engine}")
    
    pipeline = StemPipeline(base_dir=output) if output else StemPipeline()
    
    result = pipeline.separate(
        input_file,
        engine=engine,
        skip_if_exists=False,
        quality_fallback=not no_fallback
    )
    
    if result.success:
        click.echo(click.style("[OK] Separation complete!", fg="green"))
        click.echo(f"   Engine used: {result.engine_name}")
        click.echo(f"   Time: {result.processing_time_seconds:.1f}s")
        click.echo("   Stems:")
        for stem_name, path in result.stem_paths.items():
            click.echo(f"      {stem_name}: {path}")
    else:
        click.echo(click.style(f"[FAIL] Separation failed: {result.error_message}", fg="red"))
        sys.exit(1)


@cli.command()
@click.argument('input_dir', type=click.Path(exists=True))
@click.option('--output', '-o', type=click.Path(), default='data/stems',
              help='Output directory for stems')
@click.option('--limit', '-n', type=int, default=None,
              help='Maximum number of files to process')
@click.option('--skip-existing', is_flag=True, default=True,
              help='Skip files that already have stems')
def batch(input_dir: str, output: str, limit: int, skip_existing: bool):
    """
    Batch process all audio files in a directory.
    
    Scans the directory for audio files and processes them in priority order.
    """
    from ..dj.batch_processor import DJBatchProcessor
    from ..dj.library_scanner import ScannedTrack
    from ..core.stem_pipeline import StemPipeline
    
    def progress_callback(progress, track: ScannedTrack):
        status = f"[{progress.completed + progress.skipped}/{progress.total}]"
        click.echo(f"   {status} {track.display_name}")
    
    click.echo(f"[*] Batch processing: {input_dir}")
    click.echo(f"   Output: {output}")
    
    pipeline = StemPipeline(base_dir=output)
    processor = DJBatchProcessor(pipeline=pipeline)
    
    result = processor.process_directory(
        input_dir,
        progress_callback=progress_callback,
        skip_existing=skip_existing,
        limit=limit
    )
    
    click.echo("")
    click.echo(click.style("Batch Complete!", fg="green"))
    click.echo(f"   Processed: {result.progress.completed}")
    click.echo(f"   Skipped: {result.progress.skipped}")
    click.echo(f"   Failed: {result.progress.failed}")
    click.echo(f"   Time: {result.processing_time_seconds:.1f}s")
    
    if result.errors:
        click.echo(click.style("\nErrors:", fg="yellow"))
        for track, error in result.errors[:5]:  # Show first 5 errors
            click.echo(f"   {track.display_name}: {error}")


@cli.command('export-ableton')
@click.argument('stem_dir', type=click.Path(exists=True))
@click.option('--output', '-o', type=click.Path(), default=None,
              help='Output .als file path')
def export_ableton(stem_dir: str, output: str):
    """
    Export stems as an Ableton Live project.
    
    Creates a .als file with all 4 stems as separate tracks in a group.
    """
    from ..production.ableton_exporter import AbletonProjectExporter
    
    click.echo(f"[*] Exporting to Ableton: {stem_dir}")
    
    exporter = AbletonProjectExporter()
    
    try:
        als_path = exporter.export(
            Path(stem_dir),
            Path(output) if output else None
        )
        click.echo(click.style(f"[OK] Created: {als_path}", fg="green"))
    except Exception as e:
        click.echo(click.style(f"[FAIL] Export failed: {e}", fg="red"))
        sys.exit(1)


@cli.command()
@click.argument('vocal_file', type=click.Path(exists=True))
@click.option('--output', '-o', type=click.Path(), default=None,
              help='Output directory for chops')
@click.option('--max-chops', '-n', type=int, default=32,
              help='Maximum number of chops to create')
def chop(vocal_file: str, output: str, max_chops: int):
    """
    Create vocal chops from a vocal stem.
    
    Detects transients and slices into one-shot samples.
    """
    from ..production.stem_remixer import StemRemixer
    
    click.echo(f"[*] Creating chops from: {vocal_file}")
    
    remixer = StemRemixer(output_dir=output)
    
    chops = remixer.create_vocal_chops(
        Path(vocal_file),
        output_dir=Path(output) if output else None,
        max_chops=max_chops
    )
    
    if chops:
        click.echo(click.style(f"[OK] Created {len(chops)} chops", fg="green"))
        for chop in chops[:5]:
            click.echo(f"   {chop.name}")
        if len(chops) > 5:
            click.echo(f"   ... and {len(chops) - 5} more")
    else:
        click.echo(click.style("No chops created", fg="yellow"))


@cli.command()
@click.option('--clear', is_flag=True, help='Clear all cached stems')
@click.option('--older-than', type=int, default=None,
              help='Clear cache entries older than N days')
def cache(clear: bool, older_than: int):
    """
    Manage the stem cache.
    
    View cache stats or clear old entries.
    """
    from ..optimization.stem_cache import StemCache
    
    stem_cache = StemCache()
    
    if clear:
        cleared = stem_cache.clear_cache(older_than_days=older_than)
        click.echo(f"Cleared {cleared} cache entries")
    else:
        stats = stem_cache.get_cache_stats()
        click.echo("Cache Statistics:")
        click.echo(f"   Entries: {stats['total_entries']}")
        click.echo(f"   Size: {stats['total_size_mb']:.1f} MB")
        click.echo(f"   Location: {stats['cache_dir']}")


@cli.command()
def info():
    """
    Show system information and available engines.
    """
    from ..optimization.gpu_manager import GPUManager
    from ..core.engines.demucs_engine import DemucsEngine
    from ..core.engines.lalal_engine import LalalEngine
    
    click.echo("System Information")
    click.echo("=" * 40)
    
    # GPU info
    gpu = GPUManager()
    if gpu.is_cuda_available:
        gpu_info = gpu.get_gpu_info()
        if gpu_info:
            click.echo(f"GPU: {gpu_info.name}")
            click.echo(f"   VRAM: {gpu_info.vram_gb:.1f} GB")
            click.echo(f"   CUDA: {gpu_info.cuda_version}")
            click.echo(f"   Recommended batch size: {gpu.get_optimal_batch_size()}")
    else:
        click.echo("GPU: Not available (CPU mode)")
    
    click.echo("")
    click.echo("Engines")
    click.echo("-" * 40)
    
    # Demucs
    demucs = DemucsEngine()
    status = click.style("[OK] Available", fg="green") if demucs.is_available() else click.style("[X] Not installed", fg="red")
    click.echo(f"Demucs: {status}")
    
    # LALAL.AI
    lalal = LalalEngine()
    status = click.style("[OK] Configured", fg="green") if lalal.is_available() else click.style("[!] API key not set", fg="yellow")
    click.echo(f"LALAL.AI: {status}")


def main():
    """Entry point for the CLI."""
    cli()


if __name__ == '__main__':
    main()
