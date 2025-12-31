"""
Test script for album art extraction using winsdk
Run with: python test_album_art.py
"""

import asyncio

# Try winsdk first, fallback to winrt
try:
    from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
    from winsdk.windows.storage.streams import Buffer, InputStreamOptions, DataReader
    print("Using winsdk package")
except ImportError:
    from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager as MediaManager
    from winrt.windows.storage.streams import Buffer, InputStreamOptions, DataReader
    print("Using winrt package")

async def get_album_art():
    print("Requesting session manager...")
    sessions = await MediaManager.request_async()
    print("Session manager obtained")

    current_session = sessions.get_current_session()
    if not current_session:
        print("No media session found. Try playing some music first.")
        return

    print("Getting media properties...")
    props = await current_session.try_get_media_properties_async()

    print(f"Title: {props.title}")
    print(f"Artist: {props.artist}")

    if not props.thumbnail:
        print("No album art available for this track.")
        return

    print("Opening thumbnail stream...")
    thumb_stream_ref = props.thumbnail
    readable_stream = await thumb_stream_ref.open_read_async()
    print(f"Stream opened, size: {readable_stream.size} bytes")

    # Create a buffer to hold the data
    buffer = Buffer(readable_stream.size)
    print("Reading stream into buffer...")
    await readable_stream.read_async(buffer, buffer.capacity, InputStreamOptions.NONE)
    print("Buffer read complete")

    # Use a DataReader to extract the bytes from the buffer
    reader = DataReader.from_buffer(buffer)
    data = bytearray(readable_stream.size)
    reader.read_bytes(data)

    # Save the image to a file
    output_file = "test_album_art.jpg"
    with open(output_file, "wb") as f:
        f.write(data)

    print(f"\nSUCCESS! Saved album art to: {output_file}")
    print(f"Image size: {len(data)} bytes")

if __name__ == "__main__":
    print("=" * 50)
    print("Album Art Extraction Test")
    print("=" * 50)
    print()
    asyncio.run(get_album_art())
    print()
    print("=" * 50)
