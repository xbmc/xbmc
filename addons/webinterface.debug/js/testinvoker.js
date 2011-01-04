var GET_GENRES = '{"jsonrpc\": \"2.0\", "method": "AudioLibrary.GetGenres", "id": 1}';

function call(method, resultContainer) {
	jQuery.post('/jsonrpc?TestInvokerCall', method, function(data) {
		if (data) {
			$('#' + resultContainer).html(data);
		}
	}, 'html');
}