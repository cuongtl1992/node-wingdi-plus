var drawer = require("./build/Release/drawer.node");
var http = require('http')
var fs = require('fs')
const jimp = require('jimp');

fs.readFile('test.png', function (err, data) {
	jimp.read(data)
		.then(image => {
			// image = image.scale(1);
			return image.getBufferAsync(jimp.MIME_PNG);
		})
		.then(async buff => {
			// const result = await drawer.printImageFromFile("Microsoft Print to PDF", "My_print_job_1", "E:\\Projects\\PrintLib\\sample.png");
			// const result2 = await drawer.printImageFromFile("Microsoft Print to PDF", "My_print_job_2", "E:\\Projects\\PrintLib\\test.png");
			// const result3 = await drawer.printImageFromFile("Microsoft Print to PDF", "My_print_job_3", "E:\\Projects\\PrintLib\\bmp_24.bmp");
			const result4 = await drawer.printImageFromBytes("Microsoft Print to PDF", "My_print_job_1", buff, buff.length);
			// console.log(result);
			// console.log(result2);
			// console.log(result3);	
			console.log(result4);	
		})
})

